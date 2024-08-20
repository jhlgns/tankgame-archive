#include "server/client_connection_state.hpp"

#include "server/server.hpp"

class LobbyState : public ClientConnectionState {
    using ClientConnectionState::ClientConnectionState;

    void Begin() override {
        this->net_message_handlers.Add(&LobbyState::handle_ready_message, this);
        this->net_message_handlers.Add(&LobbyState::handle_leave_session, this);
    }

    void End() override {
    }

    void Tick(f32 dt) override {
    }

    StringView GetDisplayName() const override {
        return "Handshake_State";
    }

    void handle_ready_message(ReadyMessage &&message) {
        auto &con = *this->connection;

        if (!con.session_id.has_value()) {
            con.Close(false, DisconnectReason::INVALID, "No session assigned");
            return;
        }

        auto session = GetServer().TryGetSession(con.session_id.value());
        if (session == nullptr) {
            con.Close(false, DisconnectReason::INVALID, "Not in any session");
            return;
        }

        if (!session->SetPlayerReady(con)) {
            con.Close(false, DisconnectReason::INVALID, "Invalid ready state");
        }
    }

    void handle_leave_session(LeaveSessionMessage &&message) {
        auto &con = *this->connection;
        if (!con.session_id.has_value()) {
            return;
        }

        auto session = GetServer().TryGetSession(con.session_id.value());
        if (session == nullptr) {
            return;
        }

        session->Remove(con);
        con.session_id.reset();
        con.player_id.reset();

        LeaveSessionMessage response;
        con.Send(response);
    }
};

UniquePtr<ClientConnectionState> client_connection_states::MakeLobby(ClientConnection *connection) {
    return std::make_unique<LobbyState>(connection);
}
