#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "client/client_game_state.hpp"
#include "common/log.hpp"
#include "common/frame_timer.hpp"
#include <iostream>

class IngameState : public ClientState {
public:
    IngameState(Entity my_tank) {
        this->game_state.my_tank = my_tank;
    }

    void Begin() override {
        this->net_message_handlers.Add<NetMessageType::LOAD_LEVEL>(&IngameState::HandleLoadLevelMessage, this);
        this->net_message_handlers.Add<NetMessageType::GAME_COMMAND>(&IngameState::HandleGameCommandMessage, this);
        this->net_message_handlers.Add(&IngameState::HandleSetTickLengthMessage, this);
        this->net_message_handlers.Add(&IngameState::HandlePauseGameMessage, this);
        this->net_message_handlers.Add(&IngameState::HandlePingMessage, this);
        //this->net_message_handlers.add(&Ingame_State::handle_pong_message, this);

        auto &graphics_manager = GetGraphicsManager();
        //SDL_CaptureMouse(SDL_TRUE);
        //SDL_SetWindowGrab(graphics_manager.win, SDL_TRUE);
        auto window_size = graphics_manager.GetWindowSize();
    }

    void End() override {
        //SDL_CaptureMouse(SDL_FALSE);
    }

    void Input(const SDL_Event &e) override {
        this->game_state.HandleInput(this->game_state.my_tank.value(), e);

#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
#if 0 // TODO (janh)
                switch (e.key.keysym.sym) {
                    case SDLK_PLUS: {
                        Set_Time_Rate_Message message;
                        message.time_rate = get_frame_timer().time_rate + 1;
                        get_client().send(message);
                    } break;

                    case SDLK_MINUS: {
                        Set_Time_Rate_Message message;
                        message.time_rate = get_frame_timer().time_rate - 1;
                        get_client().send(message);
                    } break;

                    case SDLK_p: {
                        Pause_Game_Message message;
                        message.paused = !get_frame_timer().paused;
                        get_client().send(message);
                    } break;
                }
#endif
            }
        }
#endif
    }

    void Tick(f32 dt) override {
        this->game_state.Tick(dt);
    }

    void Render() override {
        this->game_state.Render();
    }

    StringView GetDisplayName() const override { return "IngameState"; }

    void HandleLoadLevelMessage(Packet &&packet) {
        LoadLevelMessage message;
        if (!message.Deserialize(packet) || !this->game_state.Deserialize(packet) || !packet.IsValidAndFinished()) {
            GetClient().ProtocolError();
            return;
        }

        this->game_state.cam.position =
            this->game_state.GetTankWorldPosition(this->game_state.my_tank.value()) -
            GetGraphicsManager().GetWindowSize() / 2.0f;
    }

    void HandleGameCommandMessage(Packet &&packet) {
        GameCommandMessage message;
        if (!message.Deserialize(packet)) {
            GetClient().ProtocolError();
            return;
        }

        auto success = this->game_state.HandleCommandPacket(GameState::CommandContext{}, packet);

        if (!packet.IsValidAndFinished()) {
            GetClient().ProtocolError();
            return;
        }
    }

    void HandleSetTickLengthMessage(SetTickLengthMessage &&message) {
        auto &timer = GetFrameTimer();
        timer.tick_length_delta = chrono::microseconds{message.tick_length_delta_microseconds};
        timer.tick_length_delta_end = timer.current_frame + chrono::milliseconds{message.duration_milliseconds};
        LogInfo("ingame", "Set tick length delta {} for {}"_format(
            chrono::duration_cast<std::chrono::microseconds>(GetFrameTimer().tick_length_delta),
            chrono::milliseconds{message.duration_milliseconds}));
    }

    void HandlePauseGameMessage(PauseGameMessage &&message) {
        GetFrameTimer().paused = message.paused;
        LogInfo("ingame", message.paused ? "FrameTimer paused" : "FrameTimer continued");
    }

    void HandlePingMessage(PingMessage &&message) {
        PongMessage response;
        response.my_time = this->game_state.time;
        response.your_time = message.my_time;
        GetClient().Send(response);
    }

    ClientGameState game_state;
};

UniquePtr<ClientState> client_states::MakeIngame(Entity my_tank) {
    return std::make_unique<IngameState>(my_tank);
}
