#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "common/player_info.hpp"

class LobbyState : public ClientState {
public:
    explicit LobbyState(Array<PlayerInfo> player_info) {
        this->player_info = ToRvalue(player_info);
    }

    void Begin() override {
        this->net_message_handlers.Add(&LobbyState::handle_game_started_message, this);
        this->net_message_handlers.Add(&LobbyState::handle_leave_session_message, this);
        this->net_message_handlers.Add(&LobbyState::handle_lobby_update_message, this);

        GetClient().gui.session_lobby_menu.player_info = &this->player_info;
    }

    void End() override {
        GetClient().gui.session_lobby_menu.player_info = nullptr;
    }

    void Input(const SDL_Event &e) override {
#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
                    case SDLK_RETURN:
                    case SDLK_SPACE: {
                        ReadyMessage message;
                        GetClient().Send(message);
                    } break;

                    case SDLK_ESCAPE: {
                        LeaveSessionMessage message;
                        GetClient().Send(message);
                        GetClient().SetNextState(client_states::MakeSessionbrowser());
                    } break;
                }
            }
        }
#endif
    }

    void Tick(f32 dt) override {
    }

    void Render() override {
        auto &client = GetClient();
        GetGraphicsManager().DrawBackground(client.assets.textures.lobby_background);
        client.gui.session_lobby_menu.Show();
    }

    StringView GetDisplayName() const override { return "LobbyState"; }

    void handle_game_started_message(GameStartedMessage &&message) {
        LogInfo("lobby", "Server game started, player_tank: {}"_format(message.player_tank));
        GetClient().SetNextState(client_states::MakeIngame(Entity{message.player_tank}));
    }

    void handle_leave_session_message(LeaveSessionMessage &&message) {
        GetClient().SetNextState(client_states::MakeSessionbrowser());
    }

    void handle_lobby_update_message(LobbyUpdateMessage &&message) {
        std::visit(
            Overloaded{
                [&](const LobbyUpdateMessage::PlayerLeft &player_left) {
                    for (auto &player : this->player_info) {
                        if (player.name == player_left.player_name) {
                            std::swap(player, this->player_info.back());
                            this->player_info.pop_back();
                            break;
                        }
                    }
                },
                [&](const LobbyUpdateMessage::PlayerJoined &player_joined) {
                    for (auto &player : this->player_info) {
                        if (player.name == player_joined.player_info.name) {
                            return;
                        }
                    }

                    this->player_info.emplace_back(player_joined.player_info);
                },
                [&](const LobbyUpdateMessage::UpdatePlayerInfo &update_player_info) {
                    PlayerInfo *player_info = nullptr;

                    for (auto &player : this->player_info) {
                        if (player.name == update_player_info.player_info.name) {
                            player_info = &player;
                        }
                    }

                    if (player_info == nullptr) {
                        player_info = &this->player_info.emplace_back();
                    }

                    *player_info = update_player_info.player_info;
                }
            },
            message.data);
    }

    Array<PlayerInfo> player_info;
};

UniquePtr<ClientState> client_states::MakeLobby(Array<PlayerInfo> connected_players) {
    return std::make_unique<LobbyState>(ToRvalue(connected_players));
}
