#include "common/game_state.hpp"

#include "common/net_msg.hpp"
#include "common/log.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <random>

void MoveTankCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->entity);
    packet.WriteF32(this->planet_position);
    packet.WriteF32(this->velocity);
}

bool MoveTankCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->entity) &&
        packet.ReadF32(this->planet_position) &&
        packet.ReadF32(this->velocity);
}

void RotateTurretCommand::Serialize(Packet &packet) const {
    packet.WriteB8(this->is_absolute);
    packet.WriteU32(this->entity);
    packet.WriteF32(this->target_rotation);
    packet.WriteU32(this->flags);
}

bool RotateTurretCommand::Deserialize(Packet &packet) {
    return
        packet.ReadB8(this->is_absolute) &&
        packet.ReadU32(this->entity) &&
        packet.ReadF32(this->target_rotation) &&
        packet.ReadU32(this->flags);
}

void ChargeCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->entity);
    packet.WriteB8(this->fire);
}

bool ChargeCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->entity) &&
        packet.ReadB8(this->fire);
}

void SpawnProjectileCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->target);
    packet.WriteU32(this->firing_entity);
    packet.WriteF32(this->position.x);
    packet.WriteF32(this->position.y);
    packet.WriteF32(this->velocity.x);
    packet.WriteF32(this->velocity.y);
    packet.WriteEnum(this->weapon_type);
}

bool SpawnProjectileCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->target) &&
        packet.ReadU32(this->firing_entity) &&
        packet.ReadF32(this->position.x) &&
        packet.ReadF32(this->position.y) &&
        packet.ReadF32(this->velocity.x) &&
        packet.ReadF32(this->velocity.y) &&
        packet.ReadEnum(this->weapon_type);
}

void DestroyEntityCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->target);
}

bool DestroyEntityCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->target);
}

void SetHealthCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->target);
    packet.WriteF32(this->health);
    packet.WriteF32(this->max);
}

bool SetHealthCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->target) &&
        packet.ReadF32(this->health) &&
        packet.ReadF32(this->max);
}

void PlaySfxCommand::Serialize(Packet &packet) const {
    packet.WriteEnum(this->sfx);
}

bool PlaySfxCommand::Deserialize(Packet &packet) {
    return
        packet.ReadEnum(this->sfx);
}

void SetPositionCommand::Serialize(Packet &packet) const {
    packet.WriteU32(this->target);
    packet.WriteF32(this->position.x);
    packet.WriteF32(this->position.y);
}

bool SetPositionCommand::Deserialize(Packet &packet) {
    return
        packet.ReadU32(this->target) &&
        packet.ReadF32(this->position.x) &&
        packet.ReadF32(this->position.y);
}

void SwitchWeaponCommand::Serialize(Packet &packet) const {
    packet.WriteEnum(this->weapon_type);
}

bool SwitchWeaponCommand::Deserialize(Packet &packet) {
    return
        packet.ReadEnum(this->weapon_type);
}

bool GameState::HandleCommandPacket(const CommandContext &context, Packet &packet) {
    GameCommand::Type type;

    if (!packet.ReadEnum(type)) {
        return false;
    }

#define DO_COMMAND(type_id, command_type) \
    case GameCommand::Type::type_id: { \
        command_type command; \
        if (!command.Deserialize(packet)) { \
            return false; \
        } \
        return this->HandleCommand(context, command); \
    } break; \

    switch (type) {
        DO_COMMAND(MOVE_TANK,        MoveTankCommand)
        DO_COMMAND(ROTATE_TURRET,    RotateTurretCommand)
        DO_COMMAND(CHARGE,           ChargeCommand)
        DO_COMMAND(SPAWN_PROJECTILE, SpawnProjectileCommand)
        DO_COMMAND(DESTROY_ENTITY,   DestroyEntityCommand)
        DO_COMMAND(SET_HEALTH,       SetHealthCommand)
        DO_COMMAND(PLAY_SFX,         PlaySfxCommand)
        DO_COMMAND(SET_POSITION,     SetPositionCommand)
        DO_COMMAND(SWITCH_WEAPON,    SwitchWeaponCommand)
        default:
            return false;
    }
#undef DO_COMMAND
}

void GameState::SerializeCommand(const GameCommand &command, Packet &packet) {
    packet.WriteEnum(command.type);

#define DO_COMMAND(type_id, command_type) \
    case GameCommand::Type::type_id: { \
        auto cmd = static_cast<const command_type &>(command); \
        cmd.Serialize(packet); \
    } break; \

    switch (command.type) {
        DO_COMMAND(MOVE_TANK,        MoveTankCommand)
        DO_COMMAND(ROTATE_TURRET,    RotateTurretCommand)
        DO_COMMAND(CHARGE,           ChargeCommand)
        DO_COMMAND(SPAWN_PROJECTILE, SpawnProjectileCommand)
        DO_COMMAND(DESTROY_ENTITY,   DestroyEntityCommand)
        DO_COMMAND(SET_HEALTH,       SetHealthCommand)
        DO_COMMAND(PLAY_SFX,         PlaySfxCommand)
        DO_COMMAND(SET_POSITION,     SetPositionCommand)
        DO_COMMAND(SWITCH_WEAPON,    SwitchWeaponCommand)
    }
#undef DO_COMMAND
}

Vec2 GameState::GetTankWorldPosition(Entity entity) const {
    const auto &tank = this->entities.Get<CTank>(entity);
    const auto &planet_position = this->entities.Get<CPlanetPosition>(entity);
    const auto &planet = this->entities.Get<CPlanet>(tank.planet_id);
    const auto &planet_pos = this->entities.Get<CPosition>(tank.planet_id);

    return planet_pos.value +
        (Vec2{planet.radius, planet.radius} +
            Vec2{CTank::BASE_HEIGHT, CTank::BASE_HEIGHT} / 2.0f) *
            Vec2{glm::cos(glm::radians(planet_position.value)),
                  glm::sin(glm::radians(planet_position.value))};
}

Array<Entity> GameState::Fire(Entity firing_tank, bool force) {
    Array<Entity> res;

    auto &tank = this->entities.Get<CTank>(firing_tank);
    auto &weapon = g_weapons[static_cast<size_t>(tank.weapon_type)];

    auto charging = this->entities.TryGet<CCharging>(firing_tank);
    auto charge = 1.0f;
    if (charging != nullptr) {
        charge = std::min(this->time - charging->start_time, weapon.MAX_CHARGE);
    } else if (!force) {
        //log_debug("wtf", "not charging");
        return res;
    }

    if (tank.last_fire_time + weapon.cooldown > this->time && !force) {
        //log_debug("wtf", "cooldown {}"_format(this->time - tank.last_fire_time + weapon.cooldown));
        return res;
    }

    tank.last_fire_time = this->time;

    std::uniform_real_distribution dist_spread{-weapon.spread, weapon.spread};
    std::uniform_real_distribution dist_speed_spread{-weapon.speed_spread, weapon.speed_spread};
    std::uniform_real_distribution dist_bounce{0.0f, 1.0f};

    for (size_t i = 0; i < weapon.burst; ++i) {
        auto direction = glm::rotate(Vec2{0.0f, 1.0f}, -glm::radians(tank.turret_rotation + dist_spread(this->rng)));

        auto projectile = CreateEntity(this->entities, EntityPrefabId::PROJECTILE);

        if (dist_bounce(this->rng) >= 0.95) {
            this->entities.Add<CProjectileBounce>(projectile);
        }

        auto velocity = direction * (charge / weapon.MAX_CHARGE + 0.3f) / 1.3f * (weapon.speed + dist_speed_spread(this->rng));
        auto position = this->GetTankWorldPosition(firing_tank);

        this->entities.Get<CPosition>(projectile).value = position;
        this->entities.Get<CVelocity>(projectile).value = velocity;
        this->entities.Get<CMass>(projectile).value = weapon.projectile_mass;
        this->entities.Get<CTimeToLiveBeforeExplosion>(projectile).value = weapon.projectile_ttl;
        auto &projectile_component = this->entities.Get<CProjectile>(projectile);
        projectile_component.firing_entity = firing_tank;
        projectile_component.impact_damage = weapon.damage;

        res.emplace_back(projectile);
    }

    //log_debug("wtf", "fire");

    return res;
}

Vec2 GameState::GetSunPosition() const {
    return this->size / 2.0f;
}

void GameState::Clone(GameState &target) const {
    this->entities.Each([&](Entity entity) { target.entities.Create(entity); });
    CloneRegistry(this->entities, target.entities);
    target.background_color = this->background_color;
    target.size = this->size;
    target.time = this->time;
}
