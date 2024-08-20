#include "client/client_state.hpp"

#include "client/client.hpp"
#include "common/log.hpp"

class HandshakeState : public ClientState {
    void Begin() override {
        this->net_message_handlers.Add(&HandshakeState::HandleHandshakeResponse, this);

        HandshakeRequest request;
        request.ver_major = VER_MAJOR;
        request.ver_minor = VER_MINOR;
        request.ver_build = VER_BUILD;
        GetClient().Send(request);
    }

    void End() override {
    }

    void Input(const SDL_Event &e) override {
    }

    void Tick(f32 dt) override {
    }

    void Render() override {
    }

    StringView GetDisplayName() const override { return "HandshakeState"; }

    void HandleHandshakeResponse(HandshakeResponse &&response) {
        LogInfo("handshake", "Server game version: {}.{}.{}"_format(response.ver_major, response.ver_minor, response.ver_build));
        GetClient().SetNextState(client_states::MakeSessionbrowser());
    }
};

UniquePtr<ClientState> client_states::MakeHandshake() {
    return std::make_unique<HandshakeState>();
}
