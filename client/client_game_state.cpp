#include "client/client_game_state.hpp"

#include "common/net_msg.hpp"
#include "common/log.hpp"
#include "client/client.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <random>

ClientGameState::ClientGameState() {
    this->command_callbacks[GameCommand::Type::MOVE_TANK] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &move_tank = static_cast<MoveTankCommand &>(command);
            auto entity = Entity{move_tank.entity};
            auto &planet_position = state.entities.Get<CPlanetPosition>(entity);
            planet_position.value = move_tank.planet_position;
            planet_position.delta = move_tank.velocity;
            return true;
        };

    this->command_callbacks[GameCommand::Type::ROTATE_TURRET] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &rotate_turret = static_cast<RotateTurretCommand &>(command);
            state.entities.Get<CTank>(Entity{rotate_turret.entity}).target_turret_rotation = rotate_turret.target_rotation;
            return true;
        };

    this->command_callbacks[GameCommand::Type::CHARGE] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &charge_command = static_cast<ChargeCommand &>(command);

            auto player_tank = state.my_tank.value();
            if (player_tank != Entity{charge_command.entity}) {
                return false;
            }

            auto charging = state.entities.TryGet<CCharging>(player_tank);
            if (charge_command.fire) {
                if (charging == nullptr) {
                    // Was not charging
                    return false;
                } else {
                    state.entities.Remove<CCharging>(player_tank);
                }
            } else {
                if (charging == nullptr) {
                    // Charge
                    state.entities.Add<CCharging>(player_tank).start_time = state.time;
                    return true;
                } else {
                    // Was already charging
                    charging->start_time = state.time;
                    return false;
                }
            }

            return true;
        };

    this->command_callbacks[GameCommand::Type::SPAWN_PROJECTILE] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &spawn_projectile = static_cast<SpawnProjectileCommand &>(command);
            auto projectile = CreateEntity(state.entities, EntityPrefabId::PROJECTILE, Entity{spawn_projectile.target});
            state.entities.Get<CPosition>(projectile).value = spawn_projectile.position;
            state.entities.Get<CVelocity>(projectile).value = spawn_projectile.velocity;
            state.entities.Get<CMass>(projectile).value = g_weapons[static_cast<size_t>(spawn_projectile.weapon_type)].projectile_mass;
            state.entities.Get<CProjectile>(projectile).firing_entity = Entity{spawn_projectile.firing_entity};
            //log_debug("client_game_state", "Spawn projectile");
            return true;
        };

    this->command_callbacks[GameCommand::Type::DESTROY_ENTITY] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &destroy_entity = static_cast<DestroyEntityCommand &>(command);

            //log_debug("client_game_state", "Destroy entity {}"_format(destroy_entity.target));
            // TODO(janh): I don't know what's going wrong here, but entt sometimes complains about the entity not being valid.
            // This if-condition is just a hack and probably leads to orphaned entities.
            if (state.entities.IsValid(Entity{destroy_entity.target})) {
                state.entities.Destroy(Entity{destroy_entity.target});
            }

            return true;
        };

    this->command_callbacks[GameCommand::Type::SET_HEALTH] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &set_health = static_cast<SetHealthCommand &>(command);
            auto &health = state.entities.Get<CHealth>(Entity{set_health.target});
            health.value = set_health.health;
            health.max = set_health.max;
            return true;
        };

    this->command_callbacks[GameCommand::Type::PLAY_SFX] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &play_sfx = static_cast<PlaySfxCommand &>(command);

            auto &client = GetClient();

            switch (play_sfx.sfx) {
                case PlaySfxCommand::Sfx::TANK_EXPLOSION:
                    client.PlaySample(client.assets.sounds.tank_explode);
                    return true;

                case PlaySfxCommand::Sfx::TANK_FIRE:
                    client.PlaySample(client.assets.sounds.tank_fire);
                    return true;

                default:
                    return false;
            }
        };

    this->command_callbacks[GameCommand::Type::SET_POSITION] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &set_position = static_cast<SetPositionCommand &>(command);
            auto &position = state.entities.Get<CPosition>(Entity{set_position.target});
            position.value = set_position.position;
            return true;
        };

    this->command_callbacks[GameCommand::Type::SWITCH_WEAPON] =
        [](ClientGameState &state, const CommandContext &context, GameCommand &command) {
            auto &switch_weapon = static_cast<SwitchWeaponCommand &>(command);
            auto &tank = state.entities.Get<CTank>(state.my_tank.value());
            tank.weapon_type = switch_weapon.weapon_type;
            return true;
        };
}

bool ClientGameState::Deserialize(Packet &packet) {
    return
        packet.ReadU8(this->background_color.r) &&
        packet.ReadU8(this->background_color.g) &&
        packet.ReadU8(this->background_color.b) &&
        packet.ReadU8(this->background_color.a) &&
        packet.ReadF32(this->size.x) &&
        packet.ReadF32(this->size.y) &&
        DeserializeEntities(this->entities, packet);
}

bool ClientGameState::HandleCommand(const CommandContext &context, GameCommand &command) {
    auto it = this->command_callbacks.find(command.type);
    if (it == this->command_callbacks.end()) {
        return false;
    }

    return it->second(*this, context, command);;
}

void ClientGameState::DestroyEntity(Entity entity) {
    // TODO
}

void ClientGameState::Clone(ClientGameState &target) const {
    this->GameState::Clone(target);
    target.cam = this->cam;
    target.command_callbacks = this->command_callbacks;
    target.my_tank = this->my_tank;
    target.is_pause_menu_open = this->is_pause_menu_open;
}

void ClientGameState::SimulateProjectileMovement(Vec2 direction, f32 charge, Array<Vec2> &output, size_t num_ticks) const {
    ClientGameState alternative_reality;
    this->Clone(alternative_reality);

    auto projectiles = alternative_reality.Fire(this->my_tank.value(), true);

    if (projectiles.size() != 1) {
        return;
    }

    auto projectile = projectiles[0];
    for (size_t i = 0; i < num_ticks; ++i) {
        alternative_reality.Tick(1.0f);
        auto position_after_tick = alternative_reality.entities.Get<CPosition>(projectile).value;
        output.emplace_back(position_after_tick);
    }
}
