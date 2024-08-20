#include "server/client_connection_state.hpp"

#include "server/server.hpp"
#include "server/server_game_state.hpp"
#include "server/session.hpp"
#include "common/frame_timer.hpp"

class IngameState : public ClientConnectionState {
    using ClientConnectionState::ClientConnectionState;

    void Begin() override {
        this->net_message_handlers.Add<NetMessageType::GAME_COMMAND>(&IngameState::handle_game_command, this);
        this->net_message_handlers.Add(&IngameState::handle_set_tick_length_message, this);
        this->net_message_handlers.Add(&IngameState::handle_pause_game_message, this);
        //this->net_message_handlers.add(&Ingame_State::handle_ping_message, this);
        this->net_message_handlers.Add(&IngameState::handle_pong_message, this);
    }

    void End() override {
    }

    void Tick(f32 dt) override {
        if (this->connection->session_id.has_value()) {
            auto session = GetServer().TryGetSession(this->connection->session_id.value());
            if (session && session->game_state) {
                PingMessage ping;
                ping.my_time = session->game_state->time;
                this->connection->Send(ping);
            }
        }
    }

    StringView GetDisplayName() const override {
        return "Handshake_State";
    }

    void handle_game_command(Packet &&packet) {
        auto &con = *this->connection;
        if (!con.session_id.has_value()) {
            con.Close(false, DisconnectReason::INVALID, "Can not handle game command: invalid session");
            return;
        }

        auto session = GetServer().TryGetSession(con.session_id.value());
        if (session == nullptr || session->state != SessionState::INGAME || session->game_state == nullptr) {
            con.Close(false, DisconnectReason::INVALID, "Can not handle game command: invalid session");
            return;
        }

        session->game_state->HandleCommandPacket(GameState::CommandContext{.con = &con}, packet);
    }

    void handle_set_tick_length_message(SetTickLengthMessage&& message) {
        if (this->connection->IsAdmin()) {
            auto &timer = GetFrameTimer();
            timer.tick_length_delta = chrono::microseconds{message.tick_length_delta_microseconds};
            timer.tick_length_delta_end = timer.current_frame + chrono::milliseconds{message.duration_milliseconds};
            LogInfo("ingame", "Set tick length delta: {} duration: {}"_format(
                chrono::duration_cast<chrono::microseconds>(GetFrameTimer().tick_length_delta),
                chrono::milliseconds{message.duration_milliseconds}
            ));

            if (this->connection->session_id.has_value()) {
                if (auto session = GetServer().TryGetSession(this->connection->session_id.value())) {
                    session->Broadcast(message);
                }
            }
        } else {
            this->connection->Close(false, DisconnectReason::INVALID, "Setting tick length not allowed");
        }
    }

    void handle_pause_game_message(PauseGameMessage &&message) {
        if (this->connection->IsAdmin()) {
            auto &server = GetServer();
            GetFrameTimer().paused = message.paused;

            if (this->connection->session_id.has_value()) {
                if (auto session = server.TryGetSession(this->connection->session_id.value())) {
                    session->Broadcast(message);
                }
            }
        } else {
            this->connection->Close(false, DisconnectReason::INVALID, "Pausing game not allowed");
        }
    }

#if 0
    void handle_ping_message(Ping_Message&& message) {
        Pong_Message response;
        response.my_time = get_frame_timer().time;
        response.your_time = message.my_time;
        this->connection->send(response);
    }
#endif

    void handle_pong_message(PongMessage&& message) {
        auto &con = *this->connection;

        CHECK(this->connection->session_id.has_value());
        auto session = GetServer().TryGetSession(this->connection->session_id.value());
        CHECK(session && session->game_state);
        auto time = session->game_state->time;

        auto rtt = time - message.your_time;
        con.rtt_ringbuf[con.rtt_ringbuf_pos++] = rtt;
        con.rtt_ringbuf_pos %= con.rtt_ringbuf.size();

        auto rtt_avg = 0.0f;
        for (size_t i = 0; i < con.rtt_ringbuf.size(); ++i) {
            rtt_avg += con.rtt_ringbuf[i];
        }

        rtt_avg /= con.rtt_ringbuf.size();
        //DUMP(rtt_avg);

        auto half_rtt = rtt_avg / 2.0f;
        auto server_time = message.your_time;

        // NOTE(janh): Our best guess of the client's time (in our notion of time) when the client received the ping message
        auto approx_client_time = message.my_time - half_rtt;

        // NOTE(janh): This is where the client would have been ideally when we sent the ping
        auto target = server_time + half_rtt;

        // NOTE(janh): diff < 0 -> client time ahead of server, diff > 0 -> client time behind server
        auto diff = target - approx_client_time;
        con.time_diff_ringbuf[con.time_diff_ringbuf_pos++] = diff;
        con.time_diff_ringbuf_pos %= con.time_diff_ringbuf.size();

        constexpr auto speed_change_cooldown = 9.0f;
        if (con.time_last_speed_change_requested + speed_change_cooldown < time) {
            auto diff_avg = 0.0f;
            for (size_t i = 0; i < con.time_diff_ringbuf.size(); ++i) {
                diff_avg += con.time_diff_ringbuf[i];
            }

            diff_avg /= con.time_diff_ringbuf.size();
            //log_info("client time diff", "{:.2f} ticks {}"_format(std::abs(diff_avg), diff_avg < 0.0f ? "ahead" : "behind"));

            constexpr auto punishable_offense = 50.0f;
            constexpr auto epsilon = 3.5;

#if !defined(DEVELOPMENT) || !DEVELOPMENT
            if (std::abs(diff_avg) > punishable_offense) {
                log_warn("ingame", "Kicking client that can't keep up the tick rate");
                this->connection->close(false, Disconnect_Reason::PROTO_ERR, "It looks like you could not keep up the frame rate");
                return;
            } else
#endif
            if (diff_avg > epsilon) {
                SetTickLengthMessage message;
                message.tick_length_delta_microseconds = -750; // Slower
                message.duration_milliseconds = 650; // TODO(janh): find best value
                con.Send(message);
            } else if (diff_avg < -epsilon) {
                SetTickLengthMessage message;
                message.tick_length_delta_microseconds = 750; // Faster
                message.duration_milliseconds = 650; // TODO(janh): find best value
                con.Send(message);
            }

            con.time_last_speed_change_requested = time;
        }
    }
};

UniquePtr<ClientConnectionState> client_connection_states::MakeIngame(ClientConnection *connection) {
    return std::make_unique<IngameState>(connection);
}
