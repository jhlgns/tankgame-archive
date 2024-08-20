#include "server/server_game_state.hpp"

#include "common/net_msg.hpp"
#include "common/log.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <random>

#include "server/server.hpp"
#include "server/session.hpp"

ServerGameState::ServerGameState(Session *session)
    : session(session) {
    this->command_callbacks[GameCommand::Type::MOVE_TANK] =
        [](ServerGameState &state, const CommandContext &context, GameCommand &command) {
            auto &move_tank = static_cast<MoveTankCommand &>(command);
            auto player_tank = state.session->GetPlayer(*context.con).tank_id;

            if (player_tank != Entity{move_tank.entity}) {
                return false;
            }

            auto &planet_position = state.entities.Get<CPlanetPosition>(player_tank);
            planet_position.delta = move_tank.velocity;
            move_tank.planet_position = planet_position.value;
            return true;
        };

    this->command_callbacks[GameCommand::Type::ROTATE_TURRET] =
        [](ServerGameState &state, const CommandContext &context, GameCommand &command) {
            auto &rotate_turret = static_cast<RotateTurretCommand &>(command);
            auto player_tank = state.session->GetPlayer(*context.con).tank_id;

            if (player_tank != Entity{rotate_turret.entity}) {
                return false;
            }

            if (rotate_turret.is_absolute) {
                state.entities.Get<CTank>(player_tank).target_turret_rotation = rotate_turret.target_rotation;
            } else {
                state.entities.Get<CTank>(player_tank).flags = rotate_turret.flags;
            }

            return true;
        };

    this->command_callbacks[GameCommand::Type::CHARGE] =
        [](ServerGameState &state, const CommandContext &context, GameCommand &command) {
            auto &charge_command = static_cast<ChargeCommand &>(command);
            auto player_tank = state.session->GetPlayer(*context.con).tank_id;
            if (player_tank != Entity{charge_command.entity}) {
                return false;
            }

            auto charging = state.entities.TryGet<CCharging>(player_tank);
            auto &tank = state.entities.Get<CTank>(player_tank);

            if (charge_command.fire) {
                if (charging == nullptr) {
                    // Was not charging
                    return false;
                } else if (tank.weapon_type == Weapon::Type::MACHINEGUN) {
                    // Machine gun needs no charge
                    state.entities.Remove<CCharging>(player_tank);
                    //log_debug("fire", "Stop fire machine gun");
                    return true;
                }

                // "Fall through": spawn projectile (see below)
            } else {
                if (charging == nullptr) {
                    //log_debug("fire", "Charge");
                    state.entities.Add<CCharging>(player_tank).start_time = state.time;

                    if (tank.weapon_type == Weapon::Type::MACHINEGUN) {
                        //log_debug("fire", "Start fire machine gun");
                    }

                    return true;
                } else {
                    // Was already charging
                    charging->start_time = state.time;
                    return false;
                }
            }

            auto res = state.FireProjectile(player_tank);
            state.entities.Remove<CCharging>(player_tank);
            return res;
        };

    this->command_callbacks[GameCommand::Type::SWITCH_WEAPON] =
        [](ServerGameState &state, const CommandContext &context, GameCommand &command) {
            auto &switch_weapon = static_cast<SwitchWeaponCommand &>(command);
            auto &tank = state.entities.Get<CTank>(state.session->GetPlayer(*context.con).tank_id);
            tank.weapon_type = switch_weapon.weapon_type;
            return true;
        };
}

void ServerGameState::Serialize(Packet &packet) const {
    packet.WriteU8(this->background_color.r);
    packet.WriteU8(this->background_color.g);
    packet.WriteU8(this->background_color.b);
    packet.WriteU8(this->background_color.a);
    packet.WriteF32(this->size.x);
    packet.WriteF32(this->size.y);
    SerializeEntities(this->entities, packet);
}

bool ServerGameState::HandleCommand(const CommandContext &context, GameCommand &command) {
    auto it = this->command_callbacks.find(command.type);

    if (it == this->command_callbacks.end()) {
        return false;
    }

    auto succeeded = it->second(*this, context, command);

#if SERVER
    if (succeeded) {
        GameCommandMessage message;
        Packet packet;
        message.Serialize(packet);
        this->SerializeCommand(command, packet);
        this->session->BroadcastPacket(ToRvalue(packet));
    }
#endif // SERVER

    return succeeded;
}

void ServerGameState::Prepare() {
    LogInfo("server_game_state prepare", "creating player tanks");

    constexpr Vec2 planet_padding{300.0f, 300.0f};
    constexpr Vec2 planet_spacing{480.0f, 480.0f};
    constexpr Vec2i planet_grid_size{4, 4};

    std::uniform_real_distribution dist_displacement{-170.0f, 170.0f};
    std::uniform_real_distribution dist_mass{17.0f, 32.0f};
    std::uniform_real_distribution dist_radius{70.0f, 120.0f};
    std::uniform_int_distribution  dist_color{0, 140};
    std::uniform_real_distribution dist_planet_position{0.0f, 360.0f};
    std::uniform_int_distribution  dist_background_color_red{4, 10};
    std::uniform_int_distribution  dist_background_color_green{4, 20};
    std::uniform_int_distribution  dist_background_color_blue{20, 40};

    this->background_color = Color{
        dist_background_color_red(this->rng),
        dist_background_color_green(this->rng),
        dist_background_color_blue(this->rng),
        255
    };

    auto num_planets = this->session->players.size() + this->session->num_npcs + 3;

    Array<Entity> planets;

    for (size_t i = 0; i < num_planets; ++i) {
        auto planet = CreateEntity(this->entities, EntityPrefabId::PLANET);
        planets.emplace_back(planet);

        this->size = 2.0f * planet_padding + Vec2{planet_grid_size} * planet_spacing;
        auto displacement = Vec2{dist_displacement(this->rng), dist_displacement(this->rng)};
        auto position = planet_padding + Vec2{(i % planet_grid_size.x), i / planet_grid_size.y} * planet_spacing + displacement;
        this->entities.Get<CPosition>(planet).value = position;
        this->entities.Get<CMass>(planet).value = dist_mass(this->rng);
        this->entities.Get<CPlanet>(planet).radius = dist_radius(this->rng);
        this->entities.Get<CPlanet>(planet).initial_position = position;
        //this->entities.Get<CRenderColor>(planet).value = Color{dist_color(this->rng), dist_color(this->rng), dist_color(this->rng), 255};
    }

    std::shuffle(planets.begin(), planets.end(), this->rng);

    i32 tank_index = 0;

    for (auto &player : this->session->players) {
        if (player.has_value()) {
            auto player_tank = CreateEntity(this->entities, EntityPrefabId::TANK);
            player.value().tank_id = player_tank;
            this->entities.Get<CTank>(player_tank).planet_id = planets[tank_index++];
            this->entities.Get<CPlanetPosition>(player_tank).value = dist_planet_position(this->rng);
            auto &health = this->entities.Get<CHealth>(player_tank);
            health.value = 100.0f;
            health.max = 100.0f;
        }
    }

    for (size_t i = 0; i < this->session->num_npcs; ++i) {
        auto npc_tank = CreateEntity(this->entities, EntityPrefabId::TANK);
        this->entities.Get<CTank>(npc_tank).planet_id = planets[tank_index++];
        this->entities.Get<CPlanetPosition>(npc_tank).value = dist_planet_position(this->rng);
        auto &health = this->entities.Get<CHealth>(npc_tank);
        health.value = 100.0f;
        health.max = 100.0f;
    }
}

void ServerGameState::DestroyEntity(Entity entity) {
    if (this->entities.TryGet<CTank>(entity) != nullptr) {
        PlaySfxCommand play_sfx;
        play_sfx.sfx = PlaySfxCommand::Sfx::TANK_EXPLOSION;

        GameCommandMessage play_sfx_message;
        Packet packet;
        play_sfx_message.Serialize(packet);
        this->SerializeCommand(play_sfx, packet);
        this->session->BroadcastPacket(ToRvalue(packet));
    }

    this->entities.Destroy(entity);

    DestroyEntityCommand destroy_entitiy_command;
    destroy_entitiy_command.target = entt::to_integral(entity);

    GameCommandMessage message;
    Packet packet;
    message.Serialize(packet);
    this->SerializeCommand(destroy_entitiy_command, packet);
    this->session->BroadcastPacket(ToRvalue(packet));
}

bool ServerGameState::FireProjectile(Entity firing_tank) {
    auto projectiles = this->Fire(firing_tank, false);
    if (projectiles.empty()) {
        //log_debug("projectile spawn", "no projectile");
        // Could not fire, maybe due to cooldown etc.
        return false;
    }

    //log_debug("projectile spawn", "spawn projectile");
    auto &tank = this->entities.Get<CTank>(firing_tank);

    for (const auto &projectile : projectiles) {
        // Send the spawn command
        SpawnProjectileCommand spawn_projectile_command;
        spawn_projectile_command.target = entt::to_integral(projectile);
        spawn_projectile_command.firing_entity = entt::to_integral(firing_tank);
        spawn_projectile_command.position = this->entities.Get<CPosition>(projectile).value;
        spawn_projectile_command.velocity = this->entities.Get<CVelocity>(projectile).value;
        spawn_projectile_command.weapon_type = tank.weapon_type;

        Packet spawn_projectile_packet;
        GameCommandMessage game_command_message;
        game_command_message.Serialize(spawn_projectile_packet);
        spawn_projectile_packet.WriteEnum(spawn_projectile_command.type);
        spawn_projectile_command.Serialize(spawn_projectile_packet);
        this->session->BroadcastPacket(ToRvalue(spawn_projectile_packet));
    }

    // Play sfx
    PlaySfxCommand play_sfx;
    play_sfx.sfx = PlaySfxCommand::Sfx::TANK_FIRE;

    GameCommandMessage play_sfx_message;
    Packet packet;
    play_sfx_message.Serialize(packet);
    this->SerializeCommand(play_sfx, packet);
    this->session->BroadcastPacket(ToRvalue(packet));

    return true;
}
