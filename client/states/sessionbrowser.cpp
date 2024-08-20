#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/config/config.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "server/session.hpp"

class SessionBrowserState : public ClientState {
public:
    explicit SessionBrowserState(bool show_error_message)
        : show_error_message(show_error_message) {
    }

    void Begin() override {
        this->net_message_handlers.Add(&SessionBrowserState::HandleGetSessoinInfoResponse, this);
        this->net_message_handlers.Add(&SessionBrowserState::HandleJoinSessionResponse, this);

        auto &client = GetClient();
        client.gui.session_browser_menu.session_infos = &this->session_infos;

        GetSessionInfoRequest request;
        client.Send(request);
    }

    void End() override {
        GetClient().gui.session_browser_menu.session_infos = nullptr;
    }

    void Input(const SDL_Event &e) override {
#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
                    case SDLK_RETURN:
                    case SDLK_SPACE: { // Join the first session in the session list
                        if (!this->session_infos.empty()) {
                            auto &info = this->session_infos.front();
                            JoinSessionRequest request;
                            request.session_id = info.id;
                            request.player_name = GetConfig().values.player_name;
                            request.password = "";
                            GetClient().Send(request);
                        }
                    } break;

                    case SDLK_ESCAPE: {
                        GetClient().SetNextState(client_states::MakeMenu());
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
        GetGraphicsManager().DrawBackground(client.assets.textures.sessionbrowser_background);
        client.gui.session_browser_menu.Show();

        if (this->show_error_message) {
            client.gui.ShowError();
        }
    }

    StringView GetDisplayName() const override { return "SessionBrowserState"; }

    void HandleGetSessoinInfoResponse(GetSessionInfoResponse &&response) {
        this->session_infos = ToRvalue(response.sessions);
    }

    void HandleJoinSessionResponse(JoinSessionResponse &&response) {
        if (response.result == JoinSessionResult::SUCCESS) {
            GetClient().SetNextState(client_states::MakeLobby(ToRvalue(response.connected_players)));
        } else {
            LogWarning("sessionbrowser", "Could not join session: {}"_format(ToString(response.result)));
        }
    }

    Array<SessionInfo> session_infos;
    bool show_error_message;
};

UniquePtr<ClientState> client_states::MakeSessionbrowser(bool show_error_message) {
    return std::make_unique<SessionBrowserState>(show_error_message);
}
