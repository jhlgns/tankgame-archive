#include "client/client_game_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"

#include <nuklear.h>
#include <SDL.h>

bool ClientGameState::HandleInput(Entity controlled_entity, const SDL_Event &event) {
    if (nk_window_is_any_hovered(GetClient().gui.ctx)) {
        return false;
    }

    switch (event.type) {
        case SDL_KEYDOWN: {
            switch (event.key.keysym.sym) {
                case SDLK_d: {
                    MoveTankCommand move_tank;
                    move_tank.entity = entt::to_integral(controlled_entity);
                    move_tank.velocity = -0.5f;
                    GetClient().SendGameCommand(move_tank);
                    return true;
                } break;

                case SDLK_a: {
                    MoveTankCommand move_tank;
                    move_tank.entity = entt::to_integral(controlled_entity);
                    move_tank.velocity = 0.5f;
                    GetClient().SendGameCommand(move_tank);
                    return true;
                } break;

                case SDLK_q: {
                    RotateTurretCommand rotate_turret;
                    rotate_turret.is_absolute = false;
                    rotate_turret.entity = entt::to_integral(controlled_entity);
                    rotate_turret.flags = CTank::ROTATE_TURRET_LEFT;
                    GetClient().SendGameCommand(rotate_turret);
                    return true;
                } break;

                case SDLK_e: {
                    RotateTurretCommand rotate_turret;
                    rotate_turret.is_absolute = false;
                    rotate_turret.entity = entt::to_integral(controlled_entity);
                    rotate_turret.flags = CTank::ROTATE_TURRET_RIGHT;
                    GetClient().SendGameCommand(rotate_turret);
                    return true;
                } break;

                case SDLK_TAB: {
                    auto &tank = this->entities.Get<CTank>(controlled_entity);
                    SwitchWeaponCommand switch_weapon;
                    switch_weapon.weapon_type =
                        static_cast<Weapon::Type>(
                            (static_cast<size_t>(tank.weapon_type) + 1) % static_cast<size_t>(Weapon::Type::COUNT));
                    GetClient().SendGameCommand(switch_weapon);
                    return true;
                } break;

                case SDLK_SPACE: {
                    this->is_camera_locked = !this->is_camera_locked;
                } break;

                case SDLK_ESCAPE: {
                    this->is_pause_menu_open = !this->is_pause_menu_open;

                    if (this->is_pause_menu_open) {
                        //SDL_SetWindowGrab(get_graphics_manager().win, SDL_FALSE);
                    } else {
                        //SDL_SetWindowGrab(get_graphics_manager().win, SDL_TRUE);
                    }

                    return true;
                } break;

                case SDLK_1: {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    return true;
                } break;

                case SDLK_2: {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    return true;
                } break;

                case SDLK_3: {
                    //SDL_SetWindowGrab(get_graphics_manager().win, SDL_FALSE);
                    this->cam.disable_scroll = true;
                    return true;
                } break;

                case SDLK_4: {
                    //SDL_SetWindowGrab(get_graphics_manager().win, SDL_TRUE);
                    this->cam.disable_scroll = false;
                    return true;
                } break;
            }
        } break;

        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_a) {
                // Stop the tank
                MoveTankCommand move_tank;
                move_tank.entity = entt::to_integral(controlled_entity);
                move_tank.velocity = 0.0f;
                GetClient().SendGameCommand(move_tank);
                return true;
            }
        } break;

        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                ChargeCommand charge;
                charge.entity = entt::to_integral(controlled_entity);
                charge.fire = false;
                GetClient().SendGameCommand(charge);
                return true;
            }
        } break;

        case SDL_MOUSEBUTTONUP: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                ChargeCommand charge;
                charge.entity = entt::to_integral(controlled_entity);
                charge.fire = true;
                GetClient().SendGameCommand(charge);
                return true;
            }
        } break;

        case SDL_MOUSEMOTION: {
            auto tank_position = this->GetTankWorldPosition(controlled_entity);
            auto mouse_world = this->cam.GetMouseWorldPosition();
            auto direction = glm::normalize(Vec2{mouse_world} - tank_position);
            auto angle = std::fmod(glm::degrees(std::atan2(direction.x, direction.y)) + 360.0f, 360.0f);

            RotateTurretCommand rotate_turret;
            rotate_turret.is_absolute = true;
            rotate_turret.entity = entt::to_integral(controlled_entity);
            rotate_turret.target_rotation = angle;
            GetClient().SendGameCommand(rotate_turret);
            return true;
        } break;

        case SDL_MOUSEWHEEL: {
            this->cam.Zoom(event.wheel.y);
            return true;
        } break;

        case SDL_WINDOWEVENT: {
            if (event.window.event == SDL_WINDOWEVENT_LEAVE) {
                this->cam.disable_scroll = true;
                return true;
            } else if (event.window.event == SDL_WINDOWEVENT_ENTER) {
                this->cam.disable_scroll = false;
                return true;
            }
        } break;
    }

    return false;
}
