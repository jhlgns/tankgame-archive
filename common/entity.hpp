#pragma once

#include "common/common.hpp"
#include "common/packet.hpp"
#include "common/log.hpp"
#include "common/components.hpp"

#include <random>

struct EntityRegistry {
#define DECLARE_ALIAS_NOTEMPLATE(new_name, old_name, maybe_const) \
    template<typename ...Args> \
    decltype(auto) new_name(Args &&...args) maybe_const { \
        return this->impl.old_name(std::forward<Args>(args)...); \
    }

#define DECLARE_ALIAS_TEMPLATE(new_name, old_name, maybe_const) \
    template<typename ...TemplateArgs, typename ...Args> \
    decltype(auto) new_name(Args &&...args) maybe_const { \
        return this->impl.old_name<TemplateArgs...>(std::forward<Args>(args)...); \
    }

    DECLARE_ALIAS_NOTEMPLATE(Create,       create,       );
    DECLARE_ALIAS_TEMPLATE  (Add,          emplace,      );
    DECLARE_ALIAS_TEMPLATE  (Remove,       remove,       );
    DECLARE_ALIAS_NOTEMPLATE(Destroy,      destroy,      );
    DECLARE_ALIAS_TEMPLATE  (HasComponent, has,          );
    DECLARE_ALIAS_NOTEMPLATE(IsValid,      valid,        );
    DECLARE_ALIAS_TEMPLATE  (View,         view,         );
    DECLARE_ALIAS_NOTEMPLATE(Each,         each,    const);
    DECLARE_ALIAS_TEMPLATE  (Get,          get,          );
    DECLARE_ALIAS_TEMPLATE  (TryGet,       try_get,      );
    DECLARE_ALIAS_TEMPLATE  (Get,          get,     const);
    DECLARE_ALIAS_TEMPLATE  (TryGet,       try_get, const);

#undef DECLARE_ALIAS_NOTEMPLATE
#undef DECLARE_ALIAS_TEMPLATE

    entt::registry impl;
};

enum class EntityPrefabId {
    PLANET,
    TANK,
    PROJECTILE,
};

inline Entity CreateEntity(EntityRegistry &registry, EntityPrefabId id, Optional<Entity> hint = std::nullopt) {
    Entity entity;

    if (hint.has_value()) {
        if (registry.IsValid(hint.value())) {
            LogInfo("entities", "Destroy {}"_format(entt::to_integral(hint.value())));
            registry.Destroy(hint.value());
        }

        entity = registry.Create(hint.value());
        assert(entity == hint.value());
    } else {
        entity = registry.Create();
    }

    switch (id) {
        case EntityPrefabId::PLANET: {
            registry.Add<CPlanet>(entity);
            registry.Add<CPosition>(entity);
            registry.Add<CMass>(entity);
            registry.Add<CNetReplication>(entity);
        } break;

        case EntityPrefabId::TANK: {
            registry.Add<CTank>(entity);
            registry.Add<CPlanetPosition>(entity);
            registry.Add<CHealth>(entity);
            registry.Add<CNetReplication>(entity);
        } break;

        case EntityPrefabId::PROJECTILE: {
            registry.Add<CPosition>(entity);
            registry.Add<CVelocity>(entity);
            registry.Add<CMass>(entity);
            registry.Add<CProjectile>(entity);
            registry.Add<CTimeToLiveBeforeExplosion>(entity);
            registry.Add<CNetReplication>(entity);
        } break;

        default:
            UNREACHED;
    }

    return entity;
}

inline void SerializeEntities(const EntityRegistry &registry, Packet &packet) {
    auto archive = [&](const auto &...data) {
        (packet.WriteData(&data, sizeof(data)), ...);
    };

    entt::snapshot snapshot{registry.impl};
    snapshot.entities(archive);
    snapshot.component<
        CPosition,
        CVelocity,
        CMass,
        CHealth,
        CPlanet,
        CTank,
        CPlanetPosition,
        CCharging,
        CProjectile,
        CProjectileBounce
        >(archive);
}

inline bool DeserializeEntities(EntityRegistry &registry, Packet &packet) {
    auto archive = [&](auto &...data) {
        (packet.ReadData(&data, sizeof(data)), ...);
    };

    entt::snapshot_loader loader{registry.impl};
    loader.entities(archive);
    loader.component<
        CPosition,
        CVelocity,
        CMass,
        CHealth,
        CPlanet,
        CTank,
        CPlanetPosition,
        CCharging,
        CProjectile,
        CProjectileBounce
        >(archive);
    loader.orphans();

    return packet.IsValidAndFinished();
}

template<typename Type>
void CloneComponents(const EntityRegistry &from, EntityRegistry &to) {
    auto data = from.impl.data<Type>();
    auto size = from.impl.size<Type>();

    if constexpr (ENTT_IS_EMPTY(Type)) {
        to.impl.insert<Type>(data, data + size);
    } else {
        auto raw = from.impl.raw<Type>();
        to.impl.insert<Type>(data, data + size, raw, raw + size);
    }
}

using clone_fn_type = void(const EntityRegistry &, EntityRegistry &);
inline const HashMap<EntityId, clone_fn_type *> g_clone_functions = {
    std::make_pair(entt::type_info<CPosition>::id(), &CloneComponents<CPosition>),
    std::make_pair(entt::type_info<CVelocity>::id(), &CloneComponents<CVelocity>),
    std::make_pair(entt::type_info<CMass>::id(), &CloneComponents<CMass>),
    std::make_pair(entt::type_info<CHealth>::id(), &CloneComponents<CHealth>),
    std::make_pair(entt::type_info<CPlanet>::id(), &CloneComponents<CPlanet>),
    std::make_pair(entt::type_info<CTank>::id(), &CloneComponents<CTank>),
    std::make_pair(entt::type_info<CPlanetPosition>::id(), &CloneComponents<CPlanetPosition>),
    std::make_pair(entt::type_info<CCharging>::id(), &CloneComponents<CCharging>),
    std::make_pair(entt::type_info<CProjectile>::id(), &CloneComponents<CProjectile>),
    std::make_pair(entt::type_info<CTimeToLiveBeforeExplosion>::id(), &CloneComponents<CTimeToLiveBeforeExplosion>),
    std::make_pair(entt::type_info<CNetReplication>::id(), &CloneComponents<CNetReplication>),
};

inline void CloneRegistry(const EntityRegistry &from, EntityRegistry &to) {
    from.impl.visit([&](auto type_id) {
        g_clone_functions.at(type_id)(from, to);
    });
}
