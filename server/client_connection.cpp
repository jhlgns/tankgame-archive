#include "server/client_connection.hpp"

#include "server/client_connection_state.hpp"
#include "server/server.hpp"
#include "common/log.hpp"

ClientConnection::ClientConnection(i32 id, TcpSocket &&socket)
    : id(id)
    , socket(ToRvalue(socket)) {
}

ClientConnection::~ClientConnection() = default;

void ClientConnection::Start() {
    this->SetNextState(client_connection_states::MakeHandshake(this));
}

void ClientConnection::Close(bool force, DisconnectReason reason, StringView message) {
    assert(!this->garbage);
    //assert(!this->closed); TODO(janh)

    LogInfo("client connection", "closing; reason: {}, message: '{}'"_format(static_cast<int>(reason), message));

    if (this->session_id.has_value()) {
        if (auto session = GetServer().TryGetSession(this->session_id.value())) {
            session->Remove(*this);
        }
    }

    if (force) {
        this->socket.Close(false);
        this->garbage = true;
    } else {
        DisconnectMessage disconnect_message;
        disconnect_message.reason = reason;
        disconnect_message.message = String{message};
        this->Send(disconnect_message);
        this->closed = true;
        this->closed_at = chrono::high_resolution_clock::now();
    }
}

void ClientConnection::Tick(f32 dt, bool incoming, bool outgoing) {
    if (incoming) {
        this->socket.DoRecv();
    }

    if (outgoing) {
        this->socket.DoSend();
        auto send_done = this->socket.send.queue.empty() && this->socket.send.current.empty();

        if (this->closed) {
            auto timed_out =
                chrono::high_resolution_clock::now() >
                    this->closed_at + ClientConnection::last_packet_timeout;

            if (send_done || timed_out) {
                this->socket.Close(false);
                this->garbage = true;
            }
        }
    }

    if (this->socket.state == SocketState::ERROR) {
        this->Close(false, DisconnectReason::ERROR, "Socket error");
    }

    if (this->state != nullptr)  {
        this->state->Tick(dt);

        Packet incoming_packet;
        while (this->socket.Pop(incoming_packet)) {
#if 0 && SERVER
            Net_Message_Type msg;
            std::memcpy(&msg, &incoming_packet.buf[sizeof(Packet_Header)], sizeof(msg));
            log_info("recv", "received: {}"_format((int)msg));
#endif

            if (!this->state->net_message_handlers.HandlePacket(ToRvalue(incoming_packet))) {
                this->Close(false, DisconnectReason::ERROR, "Could not find packet handler in current state");
            }
        }
    }

    if (this->next_state != nullptr) {
        if (this->state != nullptr) {
            this->state->End();
        }

        this->state = ToRvalue(this->next_state);
        this->state->Begin();
    }
}

void ClientConnection::SendPacket(Packet &&packet) {
    if (this->closed) {
        return;
    }

    packet.WriteHeader();
    this->socket.Push(ToRvalue(packet));
    GetServer().NotifySent(*this);
}

void ClientConnection::SendPacketCopy(const Packet &packet) {
    if (this->closed) {
        return;
    }

    this->socket.Push(packet);
    GetServer().NotifySent(*this);
}

void ClientConnection::SetNextState(UniquePtr<ClientConnectionState> state) {
    assert(this->next_state == nullptr);
    assert(state != nullptr);
    this->next_state = ToRvalue(state);
}
