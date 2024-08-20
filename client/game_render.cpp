#include "client/client_game_state.hpp"

#include "client/graphics/graphics_manager.hpp"
#include "client/graphics/vertex_array.hpp"
#include "client/client.hpp"

static void RenderNormalmap(ClientGameState &state, const Texture &diffuse, const Texture &normal, InstanceArray &instances, Vec2 light_position) {
    auto center = state.cam.ScreenToWorld(GetGraphicsManager().GetWindowSize() / 2.0f);
    auto &shader = GetClient().assets.shaders.normal_map;
    shader.SetUniform("DiffuseTexture",  diffuse);
    shader.SetUniform("NormalTexture",   normal);
    shader.SetUniform("ViewPos",         Vec3{center, 10000.0f});
    shader.SetUniform("Light.Position",  Vec3{light_position, 10.0f});
    shader.SetUniform("Light.Ambient",   Vec3{0.2f, 0.2f, 0.2f});
    shader.SetUniform("Light.Diffuse",   Vec3{1.0f, 1.0f, 1.0f});
    shader.SetUniform("Light.Constant",  1.0f);
    shader.SetUniform("Light.Linear",    0.0001f);
    shader.SetUniform("Light.Quadratic", 0.000001f);

    VertexArray vertex_array;
    vertex_array.AddUnitQuad();
    GetGraphicsManager().Draw(vertex_array, diffuse, state.cam, &shader, &instances);
}

static void RenderProjectiles(ClientGameState &state) {
    auto &diffuse_texture = GetClient().assets.textures.planet_diffuse;
    auto &normal_map = GetClient().assets.textures.planet_normal;

    InstanceArray instances;
    state.entities.View<CProjectile, CPosition>().each(
        [&](
            Entity entity,
            CProjectile &,
            CPosition &position) {
            constexpr auto scale = 0.005f;
            auto dest_size = Vec2{diffuse_texture.dim} * scale;
            instances.Add(position.value - dest_size / 2.0f, dest_size, 0.0f);
        });

    RenderNormalmap(state, diffuse_texture, normal_map, instances, state.GetSunPosition());
}

static void RenderBackground(ClientGameState &state) {
    //GetGraphicsManager().DrawBackground(GetClient().assets.textures.ingame_background);
    auto &graphics_manager = GetGraphicsManager();
    auto window_size = graphics_manager.GetWindowSize();
    auto &shader = GetClient().assets.shaders.ingame_background;
    shader.SetUniform("iResolution", Vec2{window_size});
    shader.SetUniform("iTime", state.time);

    VertexArray vertex_array;
    vertex_array.Add(Vec2{}, Vec2{1.0f, 1.0f}, Vec2{}, Vec2{graphics_manager.GetWindowSize()}, Color{255, 0, 0, 255});
    graphics_manager.Draw(vertex_array, GetClient().assets.textures.white_pixel, Camera{}, &shader);
}

static void RenderPlanets(ClientGameState &state) {
    auto &diffuse_texture = GetClient().assets.textures.planet_diffuse;
    auto &normal_map = GetClient().assets.textures.planet_normal;

    auto light_position = state.cam.ScreenToWorld(GetGraphicsManager().GetMousePosition()); // TODO(dvk) not final
    //log_info("Debug: ", "{}|{}"_format(light_position.x, light_position.y));
    InstanceArray instances;
    state.entities.View<CPlanet, CPosition>().each(
        [&](
            Entity entity,
            CPlanet &planet,
            CPosition &position) {
            auto dest_size = Vec2{planet.radius, planet.radius} * 2.0f;
            auto dest_position = position.value - dest_size / 2.0f;
            instances.Add(dest_position, dest_size, 0.0f); // TODO: color???

            Vec2 base{1.0f, 0.0f};
            Vec2 planet_position = position.value - light_position;

            f32 star_radius = 150.0f;
            f32 angle = 0.0f;

            // Calculate planet angle to star
            if (planet_position.y < 0) {
                angle = -glm::acos(glm::dot(base, planet_position) / (glm::length(base) * glm::length(planet_position)));
            } else {
                angle = glm::acos(glm::dot(base, planet_position) / (glm::length(base) * glm::length(planet_position)));
            }

            std::vector<Vec2> light_points;
            // Calculate lights with polar coordinates
            Vec2 light1{star_radius * glm::cos(angle + glm::radians(90.0f)), star_radius * glm::sin(angle + glm::radians(90.0f))};
            Vec2 light2{star_radius * glm::cos(angle - glm::radians(90.0f)), star_radius * glm::sin(angle - glm::radians(90.0f))};
            light_points.push_back(light1);
            light_points.push_back(light2);

            // Debug Ansicht
            VertexArray va2;
            va2.AddVertex(light1 + light_position, Color{255, 0, 0, 255});
            va2.AddVertex(light2 + light_position, Color{255, 0, 0, 255});
            //get_graphics_manager().draw_lines(va2, get_client().assets.textures.white_pixel, state.cam);

            std::vector<Tangent> tangents;
            std::vector<Vec2> planet_intersections;
            i32 i = 0;
            for(auto light_point_position : light_points) {
                auto light_point_planet_position = planet_position - light_point_position;
                //LogDebug("light_position", "{}|{}"_format(light_point_position.x, light_point_position.y));

                // Calculate tangents
                f32 divider = pow(light_point_planet_position.x, 2) - pow(planet.radius, 2);
                f32 p = -2 * light_point_planet_position.x * light_point_planet_position.y / divider;
                f32 q = (pow(light_point_planet_position.y, 2) - pow(planet.radius, 2)) / divider;
                f32 m1 = -p / 2 + std::sqrt(pow(p / 2, 2) - q);
                f32 m2 = -p / 2 - std::sqrt(pow(p / 2, 2) - q);

                //f32 x0, y0, b1, b2;
                //Tangent tangent1(m1, light_point_position.y);


                // Debug Ansicht
                f32 x1, x2, y11, y12, y21, y22;
                x1 = 10000;
                x2 = -10000;
                y11 = m1 * x1;
                y12 = m1 * x2;
                y21 = m2 * x1;
                y22 = m2 * x2;
                VertexArray va;
                va.AddVertex(Vec2{x1, y11} + light_position + light_point_position, Color{});
                va.AddVertex(Vec2{x2, y12} + light_position + light_point_position, Color{});
                va.AddVertex(Vec2{x1, y21} + light_position + light_point_position, Color{});
                va.AddVertex(Vec2{x2, y22} + light_position + light_point_position, Color{});
                //get_graphics_manager().draw_lines(va, get_client().assets.textures.white_pixel, state.cam);

                // Calculate tangents intersections point with circle
                f32 b1, b2, intersection1_x, intersection1_y, intersection2_x, intersection2_y;
                b1 = planet_position.y + 1.0f / m1 * planet_position.x;
                b2 = planet_position.y + 1.0f / m2 * planet_position.x;
                intersection1_x = b1 / (1.0f + 1.0f / m1);
                intersection1_y = m1 * intersection1_x;
                intersection2_x = b2 / (1.0f + 1.0f / m2);
                intersection2_y = m2 * intersection2_x;

                planet_intersections.push_back(Vec2{intersection1_x, intersection1_y});
                planet_intersections.push_back(Vec2{intersection2_x, intersection2_y});

                VertexArray va5;
                va5.AddVertex(planet_intersections[i] + light_position, Color{255, 0, 0, 255});
                va5.AddVertex(planet_intersections[i + 1] + light_position, Color{255, 0, 0, 255});
                //get_graphics_manager().draw_lines(va5, get_client().assets.textures.white_pixel, state.cam);

                //Debug Ansicht
                VertexArray va1;
                y11 = -1.0f / m1 * x1 + b1;
                y12 = -1.0f / m1 * x2 + b1;
                y21 = -1.0f / m2 * x1 + b2;
                y22 = -1.0f / m2 * x2 + b2;
                va1.AddVertex(Vec2{x1, y11} + light_position, Color{});
                va1.AddVertex(Vec2{x2, y12} + light_position, Color{});
                va1.AddVertex(Vec2{x1, y21} + light_position, Color{});
                va1.AddVertex(Vec2{x2, y22} + light_position, Color{});
                //GetGraphicsManager().DrawLines(va1, GetClient().assets.textures.white_pixel, state.cam);
                ++i;
            }
//=======
            //instances.Add(dest_position, dest_size, 0.0f); // TODO: color???
//>>>>>>> a6b78a1... New code style
        });

    RenderNormalmap(state, diffuse_texture, normal_map, instances, state.GetSunPosition());
}

static void RenderTanks(ClientGameState &state) {
    auto &tank_diffuse_texture = GetClient().assets.textures.tank_diffuse;
    auto &tank_normal_map = GetClient().assets.textures.tank_normal;
    auto &tank_turret_diffuse_texture = GetClient().assets.textures.tank_turret_diffuse;
    auto &tank_turret_normal_map = GetClient().assets.textures.tank_turret_normal;

    InstanceArray tank_instances;
    InstanceArray turret_instances;
    auto tank_aspect = CTank::BASE_HEIGHT / tank_diffuse_texture.dim.y;
    auto turret_aspect = CTank::TURRET_HEIGHT / tank_turret_diffuse_texture.dim.y;

    state.entities.View<CTank, CPlanetPosition>().each(
        [&](
            Entity entity,
            CTank &tank,
            CPlanetPosition &planet_position) {
                Vec2 tank_dest_size{tank_diffuse_texture.dim.x * tank_aspect, CTank::BASE_HEIGHT};
                Vec2 turret_dest_size{tank_turret_diffuse_texture.dim.x * turret_aspect, CTank::TURRET_HEIGHT};
                auto dest_position = state.GetTankWorldPosition(entity) - tank_dest_size / 2.0f;
                auto turret_dest_position = state.GetTankWorldPosition(entity) - Vec2{turret_dest_size.x / 2.0f, 0.0f};

                tank_instances.Add(dest_position, tank_dest_size, planet_position.value - 90.0f);
                turret_instances.Add(turret_dest_position, turret_dest_size, Vec2{turret_dest_size.x / 2.0f, 0.0f}, -tank.turret_rotation);
        });

    RenderNormalmap(state, tank_turret_diffuse_texture, tank_turret_normal_map, turret_instances, state.GetSunPosition());
    RenderNormalmap(state, tank_diffuse_texture, tank_normal_map, tank_instances, state.GetSunPosition());
}

constexpr Vec2 healthbar_size{100.0f, 10.0f};
constexpr Vec2 healthbar_offset{-healthbar_size.x / 2.0f, 35.0f};
constexpr Vec2 fuelbar_size{healthbar_size};
constexpr Vec2 fuelbar_offset{-healthbar_size.x / 2.0f, healthbar_offset.y + healthbar_size.y + 5.0f};

static void RenderHealthBars(ClientGameState &state) {
    auto &texture = GetClient().assets.textures.white_pixel;
    VertexArray vertex_array;

    state.entities.View<CTank, CPlanetPosition, CHealth>().each(
        [&](
            Entity entity,
            CTank &tank,
            CPlanetPosition &planet_position,
            CHealth &health) {
            auto tank_position = state.GetTankWorldPosition(entity);
            auto green_width = health.value / health.max * healthbar_size.x;
            vertex_array.Add(
                Vec2{},
                Vec2{1.0f, 1.0f},
                Vec2{tank_position.x, tank_position.y} + healthbar_offset,
                Vec2{green_width, healthbar_size.y},
                Color{80, 170, 70, 255});

            auto red_width = healthbar_size.x - green_width;
            vertex_array.Add(
                Vec2{},
                Vec2{1.0f, 1.0f},
                Vec2{tank_position.x + green_width, tank_position.y} + healthbar_offset,
                Vec2{red_width, healthbar_size.y},
                Color{170, 70, 60, 255});
        });

    GetGraphicsManager().Draw(vertex_array, texture, state.cam);
}

static void RenderFuel(ClientGameState &state) {
    if (!state.entities.IsValid(state.my_tank.value())) {
        return;
    }

    auto &texture = GetClient().assets.textures.white_pixel;
    VertexArray vertex_array;

    auto &tank = state.entities.Get<CTank>(state.my_tank.value());
    auto tank_position = state.GetTankWorldPosition(state.my_tank.value());

    auto green_width = tank.fuel / CTank::MAX_FUEL * fuelbar_size.x;
    vertex_array.Add(
        Vec2{},
        Vec2{1.0f, 1.0f},
        Vec2{tank_position.x, tank_position.y} + fuelbar_offset,
        Vec2{green_width, fuelbar_size.y},
        Color{100, 50, 50, 255});

    auto red_width = fuelbar_size.x - green_width;
    vertex_array.Add(
        Vec2{},
        Vec2{1.0f, 1.0f},
        Vec2{tank_position.x + green_width, tank_position.y} + fuelbar_offset,
        Vec2{red_width, fuelbar_size.y},
        Color{255, 255, 255, 255});

    GetGraphicsManager().Draw(vertex_array, texture, state.cam);
}

void RenderAimGuide(ClientGameState &state) {
    return;
    Array<Vec2> path;
    state.SimulateProjectileMovement(Vec2{1.0f, 1.0f}, 1.0f, path, 25);

    /*Vertex_Array vertex_array;
    vertex_array.create_default_quad();

    for (auto &vertex : vertex_array.vertices) {
        vertex.color = Color{255, 0, 0, 255};
    }*/

    InstanceArray instances;

    //get_graphics_manager().draw(vertex_array, instances, get_client().assets.textures.white_pixel, this->cam);
    auto &diffuse_texture = GetClient().assets.textures.planet_diffuse;
    auto &normal_map = GetClient().assets.textures.planet_normal;
    for (const auto &position : path) {
        Mat4 model{1.0};
        model = glm::translate(model, Vec3{position, 0.0f});
        model = glm::scale(model, Vec3{4.0f, 4.0f, 1.0f});
        instances.Add(model);
    }

    RenderNormalmap(state, diffuse_texture, normal_map, instances, state.GetSunPosition());
}

void RenderHud(ClientGameState &state) {
    auto &font = GetClient().assets.fonts.default_font;
    auto window_size = GetGraphicsManager().GetWindowSize();
    auto position = Vec2{0.0f, window_size.y - font.line_spacing};
    auto &tank = state.entities.Get<CTank>(state.my_tank.value());
    auto &weapon = g_weapons[static_cast<size_t>(tank.weapon_type)];

    DrawString(font, position, weapon.name, Color{255, 255, 127, 255});
}

void ClientGameState::Render() {
    //get_graphics_manager().clear_color = this->background_color;
    RenderBackground(*this);
    RenderPlanets(*this);
    RenderTanks(*this);
    RenderProjectiles(*this);
    RenderHealthBars(*this);
    RenderFuel(*this);
    RenderAimGuide(*this);
    RenderHud(*this);

    if (this->is_pause_menu_open) {
        GetClient().gui.pause_menu.Show(*this);
    }
}
