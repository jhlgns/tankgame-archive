#include "common/game_state.hpp"

struct Session;

struct ServerGameState : public GameState {
    using Command_Callback = bool(ServerGameState &, const CommandContext &, GameCommand &);
    using Command_Callback_Map = HashMap<GameCommand::Type, Command_Callback *>;

    ServerGameState(Session *session);
    void Serialize(Packet &packet) const;
    bool HandleCommand(const CommandContext &context, GameCommand &command) final;
    void Prepare();
    void DestroyEntity(Entity entity) final;
    bool FireProjectile(Entity firing_tank);

    Command_Callback_Map command_callbacks;
    Session *session = nullptr;
};
