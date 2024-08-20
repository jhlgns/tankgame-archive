#include "server/client_connection_state.hpp"

#include "server/server.hpp"

struct HandshakeState : public ClientConnectionState {
    using ClientConnectionState::ClientConnectionState;

    void Begin() override {
        this->net_message_handlers.Add(&HandshakeState::handle_handshake, this);
    }

    void End() override {
    }

    void Tick(f32 dt) override {
    }

    StringView GetDisplayName() const override {
        return "Handshake_State";
    }

    void handle_handshake(HandshakeRequest &&request) {
        auto &con = *this->connection;
        HandshakeResponse response;
        response.ver_major = VER_MAJOR;
        response.ver_minor = VER_MINOR;
        response.ver_build = VER_BUILD;
        response.ok =
            request.ver_major == VER_MAJOR &&
            request.ver_minor == VER_MINOR &&
            request.ver_build == VER_BUILD;
        con.Send(response);

        if (!response.ok) {
            GetServer().ProtoErr(con);
        }

        con.SetNextState(client_connection_states::MakeJoinSession(&con));
    }
};


UniquePtr<ClientConnectionState> client_connection_states::MakeHandshake(ClientConnection *connection) {
    return std::make_unique<HandshakeState>(connection);
}
