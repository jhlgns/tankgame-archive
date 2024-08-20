#include "common/game_state.hpp"

#include "common/entity.hpp"
#include "common/net_msg.hpp"
#include "common/log.hpp"

#if CLIENT
#include "client/client.hpp"
#include "client/client_game_state.hpp"
#include "client/graphics/graphics_manager.hpp"
#include <SDL.h>
#endif // CLIENT

#ifdef SERVER
#include "server/server_game_state.hpp"
#include "server/session.hpp"
#endif // SERVER

void GameState::Tick(f32 dt) {
    //LogDebug("game_state time", "{}"_format(this->time));
    this->time += dt;

#if CLIENT
    auto client_game_state = static_cast<ClientGameState *>(this);
    client_game_state->cam.Update();

    if (client_game_state->is_camera_locked) {
        client_game_state->cam.position =
            client_game_state->GetTankWorldPosition(client_game_state->my_tank.value()) -
            GetGraphicsManager().GetWindowSize() / 2.0f;
    }
#endif // CLIENT

#ifdef SERVER
    this->entities.View<CTank, CCharging>().each(
        [&](Entity entity, CTank &tank, CCharging &charging) {
            if (tank.weapon_type == Weapon::Type::MACHINEGUN) {
                static_cast<ServerGameState *>(this)->FireProjectile(entity);
            }
        });
#endif

    // Update positions
    this->entities.View<CPosition, CVelocity>().each(
        [&](Entity entity, CPosition &position, CVelocity &velocity) {
            position.value += velocity.value * dt;
        });

    // Move tanks
    this->entities.View<CPlanetPosition, CTank>().each(
        [&](Entity entity, CPlanetPosition &planet_position, CTank &tank) {
            if (tank.fuel > 0.0f && planet_position.delta != 0.0f) {
                auto &planet = this->entities.Get<CPlanet>(Entity{tank.planet_id});
                auto circumference = planet.radius * 2.0f * glm::pi<f32>();
                planet_position.value += planet_position.delta * dt / circumference * 1000.0f;
                tank.fuel = std::max(0.0f, tank.fuel - dt);
                //log_debug("fuel left", "{}"_format(tank.fuel));
            }
        });

    // Update turret rotations
    this->entities.View<CTank>().each(
        [&](Entity entity, CTank &tank) {
            auto d_rotation = 0.0f;
            auto dist = std::abs(tank.turret_rotation - tank.target_turret_rotation);

            if (tank.flags == 0 && dist >= 0.01f) {
                if (tank.turret_rotation < tank.target_turret_rotation) {
                    if (dist < 180.0f) {
                        d_rotation = dt;
                    } else {
                        d_rotation = -dt;
                    }
                } else {
                    if (dist < 180.0f) {
                        d_rotation = -dt;
                    } else {
                        d_rotation = dt;
                    }
                }
            } else {
                if (tank.flags & CTank::ROTATE_TURRET_LEFT) {
                    d_rotation = dt;
                } else if (tank.flags & CTank::ROTATE_TURRET_RIGHT) {
                    d_rotation = -dt;
                }
            }

            tank.turret_rotation += d_rotation;
            if (tank.turret_rotation < 0.0f) {
                tank.turret_rotation += 360.0f;
            }

            tank.turret_rotation = std::fmod(tank.turret_rotation, 360.0f);
            //assert(current >= 0.0f);
        });

    // Gravity simulation
    this->entities.View<CPosition, CVelocity, CMass>().each(
        [&](
            Entity moving_entity,
            CPosition &moving_position,
            CVelocity &moving_velocity,
            CMass &moving_mass) {
            Vec2 force{};

            this->entities.View<CMass, CPosition>().each(
                [&](
                    Entity test_entity,
                    CMass &test_mass,
                    CPosition test_position) {
                    if (test_entity == moving_entity) {
                        return;
                    }

                    auto diff = test_position.value - moving_position.value;
                    auto dist = glm::length(diff);
                    force += 0.0000001f * diff * moving_mass.value * test_mass.value / dist * dist;
                });

            moving_velocity.value += force;
        });

#if 1
    // Rotate planets around sun
    this->entities.View<CPlanet, CPosition>().each(
        [&](
            Entity planet_entity,
            CPlanet &planet,
            CPosition &position) {
            auto sun_position = this->size / 2.0f;
            position.value = glm::rotate(planet.initial_position - sun_position, this->time * planet.orbital_velocity) + sun_position;
        });
#endif

#if SERVER
    // Projectile collision checking
    this->entities.View<CProjectile, CPosition, CVelocity>().each(
        [&](Entity projectile_entity, CProjectile &projectile, CPosition &projectile_position, CVelocity &projectile_velocity) {
        // Projectile - Tank
        this->entities.View<CTank, CHealth>().each(
            [&](Entity tank_entity, CTank &tank, CHealth &health) {
                if (tank_entity == projectile.firing_entity) {
                    return;
                }

                auto tank_position = this->GetTankWorldPosition(tank_entity);
                auto diff = tank_position - projectile_position.value;

                if (diff.x * diff.x + diff.y * diff.y < projectile.hit_radius * projectile.hit_radius) {
                    health.value -= projectile.impact_damage;
                    this->DestroyEntity(projectile_entity);

                    SetHealthCommand command;
                    command.target = entt::to_integral(tank_entity);
                    command.health = health.value;
                    command.max = health.max;

                    GameCommandMessage message;
                    Packet packet;
                    message.Serialize(packet);
                    this->SerializeCommand(command, packet);
                    static_cast<ServerGameState *>(this)->session->BroadcastPacket(ToRvalue(packet));
                }
            });

        // Projectile - Planet
        this->entities.View<CPlanet, CPosition>().each(
            [&](Entity planet_entity, CPlanet &planet, CPosition &planet_position) {
                auto diff = planet_position.value - projectile_position.value;
                auto collision_radius = planet.radius + projectile.radius;

                if (diff.x * diff.x + diff.y * diff.y < collision_radius * collision_radius) {
                    if (this->entities.HasComponent<CProjectileBounce>(projectile_entity)) {
                        this->entities.Remove<CProjectileBounce>(projectile_entity);
                        // TODO calculate bounce velocity???????????????????????
                        //TODO;
                    } else {
                        this->DestroyEntity(projectile_entity);
                    }
                }
            });
        });
#endif // SERVER

#if SERVER
    // Check time to live before explosion
    this->entities.View<CTimeToLiveBeforeExplosion>().each(
        [&](Entity entity, CTimeToLiveBeforeExplosion &ttl) {
            ttl.value -= dt;

            if (ttl.value <= 0.0f) {
                // TODO
                this->DestroyEntity(entity);
            }

        });

    // Destroy dead entities
    this->entities.View<CHealth>().each(
        [&](Entity entity, CHealth &health) {
            if (health.value <= 0.0f) {
                this->DestroyEntity(entity);
            }
        });

#if 0
    // Replicate entities
    this->entities.view<Net_Replication_Component, Position_Component>().each([&](
        Entity entity,
        Net_Replication_Component &replication,
        Position_Component &position) {
            if (replication.last_replication + 30.0f < this->time) {
                Set_Position_Command command;
                command.target = entt::to_integral(entity);
                command.position = position.value;
                Game_Command_Message message;
                Packet packet;
                message.serialize(packet);
                this->serialize_command(command, packet);
                static_cast<Server_Game_State *>(this)->session->broadcast_packet(ToRvalue(packet));

                replication.last_replication = this->time;
            }
        });
#endif

#endif // SERVER
}
