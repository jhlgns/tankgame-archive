#pragma once

#include "common/packet.hpp"
#include "common/entity.hpp"
#include "common/crc32.hpp"

struct ClientConnection;

struct GameCommand {
public:
    enum class Type : u8 {
        MOVE_TANK        = 1,
        ROTATE_TURRET    = 2,
        CHARGE           = 3,
        SPAWN_PROJECTILE = 4,
        DESTROY_ENTITY   = 5,
        SET_HEALTH       = 6,
        PLAY_SFX         = 7,
        SET_POSITION     = 8,
        SWITCH_WEAPON    = 9,
    };

    Type type;

    inline explicit GameCommand(Type type) : type(type) {}
};

struct MoveTankCommand : public GameCommand {
    inline MoveTankCommand() : GameCommand(GameCommand::Type::MOVE_TANK) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId entity = 0;
    f32 planet_position = 0.0f;
    f32 velocity = 0.0f;
};

struct RotateTurretCommand : public GameCommand {
    inline RotateTurretCommand() : GameCommand(GameCommand::Type::ROTATE_TURRET) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    bool is_absolute = true;
    EntityId entity = 0;
    f32 target_rotation = 0.0f;
    u32 flags = 0;
};

struct ChargeCommand : public GameCommand {
    inline ChargeCommand() : GameCommand(GameCommand::Type::CHARGE) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId entity = 0;
    bool fire = false;
};

struct SpawnProjectileCommand : public GameCommand {
    inline SpawnProjectileCommand() : GameCommand(GameCommand::Type::SPAWN_PROJECTILE) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId target = 0;
    EntityId firing_entity = 0;
    Vec2 position{};
    Vec2 velocity{};
    Weapon::Type weapon_type;
};

struct DestroyEntityCommand : public GameCommand {
    inline DestroyEntityCommand() : GameCommand(GameCommand::Type::DESTROY_ENTITY) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId target = 0;
};

struct SetHealthCommand : public GameCommand {
    inline SetHealthCommand() : GameCommand(GameCommand::Type::SET_HEALTH) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId target = 0;
    f32 health = 0.0f;
    f32 max = 0.0f;
};

struct PlaySfxCommand : public GameCommand {
    enum class Sfx {
        NONE,
        TANK_EXPLOSION,
        TANK_FIRE,
    };

    inline PlaySfxCommand() : GameCommand(GameCommand::Type::PLAY_SFX) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    Sfx sfx = Sfx::NONE;
};

struct SetPositionCommand : public GameCommand {
    inline SetPositionCommand() : GameCommand(GameCommand::Type::SET_POSITION) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    EntityId target = 0;
    Vec2 position{};
};

struct SwitchWeaponCommand : public GameCommand {
    inline SwitchWeaponCommand() : GameCommand(GameCommand::Type::SWITCH_WEAPON) {}

    void Serialize(Packet &packet) const;
    bool Deserialize(Packet &packet);

    Weapon::Type weapon_type = Weapon::Type::MACHINEGUN;
};

struct GameState {
    struct CommandContext {
        ClientConnection *con = nullptr;
    };

    void Tick(f32 dt);
    void SerializeCommand(const GameCommand &command, Packet &packet);
    bool HandleCommandPacket(const CommandContext &context, Packet &packet);
    virtual bool HandleCommand(const CommandContext &context, GameCommand &command) = 0;
    Vec2 GetTankWorldPosition(Entity entity) const;
    Array<Entity> Fire(Entity firing_tank, bool force);
    Vec2 GetSunPosition() const;
    virtual void DestroyEntity(Entity entity) = 0;
    void Clone(GameState &target) const;

    EntityRegistry entities;
    Color background_color;
    Vec2 size;
    f32 time = 0.0f;
    std::mt19937 rng{std::random_device{}()};
};
