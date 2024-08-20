#pragma once

#include "common/common.hpp"
#include <entt/entt.hpp>

using Entity = entt::entity;
using EntityId = entt::id_type;

struct CPosition {
    Vec2 value;
};

struct CVelocity {
    Vec2 value;
};

struct CMass {
    f32 value;
};

struct CHealth {
    f32 value;
    f32 max;
};

struct CPlanet {
    Vec2 initial_position;
    f32 radius;
    f32 orbital_velocity = 0.001f;
};

struct Weapon {
    enum class Type {
        SHOTGUN,
        MACHINEGUN,
        MISSILE,
        MORTAR,
        COUNT,
    };

    constexpr static f32 MAX_CHARGE = 60.0f;

    f32 projectile_mass;
    f32 cooldown;
    f32 damage;
    f32 speed;
    f32 projectile_ttl;
    f32 spread;
    f32 speed_spread;
    u32 burst;
    StringView name;
};

constexpr Weapon g_weapons[static_cast<size_t>(Weapon::Type::COUNT)] {
    /* WEAPON TABLE
     |mass    |cooldown |damage  |speed  |ttl     |spread |speed_spread |burst |name */
    {7.0f,    60.0f,    3.5f,    25.0f,  100.0f,  3.0f,   1.0f,         10,    "Shotgun", },
    {5.0f,    4.5f,     6.0f,    25.0f,  60.0f,   6.0f,   2.0f,         1,     "Machinegun", },
    {10.0f,   150.0f,   40.0f,   17.0f,  300.0f,  0.0f,   0.0f,         1,     "Missile launcher", },
    {12.0f,   270.0f,   50.0f,   3.0f,   400.0f,  0.0f,   0.0f,         1,     "Mortar", },
};

struct CTank {
    constexpr static f32 BASE_HEIGHT = 30.0f;
    constexpr static f32 TURRET_HEIGHT = 40.0f;
    constexpr static f32 MAX_FUEL = 1000.0f;
    constexpr static u32 ROTATE_TURRET_LEFT  = 1 << 0;
    constexpr static u32 ROTATE_TURRET_RIGHT = 1 << 1;
    f32 turret_rotation;
    f32 target_turret_rotation;
    u32 flags = 0;
    Entity planet_id;
    f32 fuel = MAX_FUEL;
    Weapon::Type weapon_type = Weapon::Type::MORTAR;
    f32 last_fire_time = 0.0f;
};

struct CPlanetPosition {
    f32 delta;
    f32 value;
};

struct CCharging {
    f32 start_time = 0.0f;
};

struct CProjectile {
    Entity firing_entity;
    Color color; // TODO only for debugging purposes
    f32 impact_damage = 0.0f;
    f32 hit_radius = 40.0f;
    f32 radius = 7.0f;
};

struct CProjectileBounce {
};

struct CTimeToLiveBeforeExplosion {
    f32 value;
};

struct CNetReplication {
    f32 last_replication;
};
