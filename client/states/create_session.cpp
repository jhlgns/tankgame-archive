#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/config/config.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "server/session.hpp"

class CreateSessionState : public ClientState {
    void Begin() override {
        this->net_message_handlers.Add(&CreateSessionState::HandleCreateSessionResponse, this);
    }

    void End() override {
    }

    void Input(const SDL_Event &e) override {
#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
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
        client.gui.create_session_menu.Show();
    }

    StringView GetDisplayName() const override { return "CreateSessionState"; }

    void HandleCreateSessionResponse(CreateSessionResponse &&response) {
        auto &client = GetClient();

        if (!response.success) {
            client.error_message = "Cannot create session";
            client.SetNextState(client_states::MakeSessionbrowser(true));
            return;
        }

        // We do this state change immediately so the join session response is handled correctly
        client.SetNextState(client_states::MakeSessionbrowser());
        JoinSessionRequest request;
        request.session_id = response.created_session_id;
        request.player_name = GetConfig().values.player_name;
        //request.password = client.gui.create_session_menu.password; TODO
        client.Send(request);
    }

    Array<SessionInfo> session_infos;
};

UniquePtr<ClientState> client_states::MakeCreateSession() {
    return std::make_unique<CreateSessionState>();
}
