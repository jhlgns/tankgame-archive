#include "server/client_connection_state.hpp"

#include "server/server.hpp"

class Join_Session_State : public ClientConnectionState {
    using ClientConnectionState::ClientConnectionState;

    void Begin() override {
        this->net_message_handlers.Add(&Join_Session_State::handle_get_session_info_request, this);
        this->net_message_handlers.Add(&Join_Session_State::handle_join_session_request, this);
        this->net_message_handlers.Add(&Join_Session_State::handle_create_session_request, this);
    }

    void End() override {
    }

    void Tick(f32 dt) override {
    }

    StringView GetDisplayName() const override {
        return "Join_Session_State";
    }

    void handle_get_session_info_request(GetSessionInfoRequest &&request) {
        GetSessionInfoResponse response;
        GetServer().GetInfo(response);
        this->connection->Send(response);
    }

    void handle_create_session_request(CreateSessionRequest &&request) {
        auto session_id =
            GetServer().CreateSession(
                request.name,
                request.password,
                request.num_players,
                request.num_bots,
                false);

        CreateSessionResponse response;
        response.created_session_id = session_id.has_value() ? session_id.value() : ~0;
        response.success = session_id.has_value();
        this->connection->Send(response);
    }

    void handle_join_session_request(JoinSessionRequest &&request) {
        auto &con = *this->connection;
        
        JoinSessionResponse response;
        response.result = JoinSessionResult::NOT_FOUND;

        auto session = GetServer().TryGetSession(request.session_id);
        if (session != nullptr) {
            response.result = session->Join(con, request.player_name, request.password);

            for (const auto &player : session->players) {
                if (player.has_value()) {
                    auto &player = session->GetPlayer(*this->connection);
                    auto player_info = session->GetPlayerInfo(player);
                    response.connected_players.emplace_back(ToRvalue(player_info));
                }
            }
        }

        con.Send(response);

        if (response.result == JoinSessionResult::SUCCESS) {
            con.SetNextState(client_connection_states::MakeLobby(&con));
        }
    }
};

UniquePtr<ClientConnectionState> client_connection_states::MakeJoinSession(ClientConnection *connection) {
    return std::make_unique<Join_Session_State>(connection);
}
