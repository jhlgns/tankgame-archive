#include "common/game_state.hpp"

#include "client/graphics/camera.hpp"

union SDL_Event;

struct ClientGameState : public GameState {
    using CommandCallback = bool(ClientGameState &, const CommandContext &, GameCommand &);
    using CommandCallbackMap = HashMap<GameCommand::Type, CommandCallback *>;

    ClientGameState();
    bool Deserialize(Packet &packet);
    bool HandleInput(Entity controlled_entity, const SDL_Event &event);
    bool HandleCommand(const CommandContext &context, GameCommand &command) final;
    void Render();
    void DestroyEntity(Entity entity) final;
    void Clone(ClientGameState &target) const;
    void SimulateProjectileMovement(Vec2 direction, f32 charge, Array<Vec2> &output, size_t num_ticks) const;

    Camera cam;
    CommandCallbackMap command_callbacks;
    Optional<Entity> my_tank;
    bool is_pause_menu_open = false;
    bool is_camera_locked = false;
};
