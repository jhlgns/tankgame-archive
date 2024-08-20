#include "common/socket.hpp"

#include "common/log.hpp"

constexpr u32 max_packet_size = 1'000'000;

SocketStats TcpSocket::global_stats;

TcpSocket::~TcpSocket() {
    this->Close(false);
}

void TcpSocket::Close(bool error) {
    if (this->sd != -1) {
        net::CloseSocket(this->sd);
        this->sd = -1;
    }

    this->state = error ? SocketState::ERROR : SocketState::NONE;
    this->remote_address = {};
    this->send.Reset();
    this->recv.Reset();
}

void TcpSocket::Connect(sockaddr_in remote_address) {
    this->Close(false);

    assert(this->state != SocketState::CONNECTING);

    this->remote_address = remote_address;
    this->sd = net::CreateNonBlockingSocket();

    if (this->sd == -1) {
        LogInfo("socket", "Cannot create socket: {}"_format(net::GetErrorString()));
        this->state = SocketState::ERROR;
        return;
    }

    this->state = SocketState::CONNECTING;

    auto result = ::connect(this->sd, reinterpret_cast<const sockaddr *>(&this->remote_address), sizeof(this->remote_address));

    if (result == 0) {
        ++this->stats.num_connections;
        ++TcpSocket::global_stats.num_connections;
        this->state = SocketState::CONNECTED;
    } else if (result == -1) {
        if (!net::IsEWouldBlockOrEInprogress()) {
            this->Close(true);
            return;
        }
    } else {
        assert(false);
    }
}

void TcpSocket::SetConnectedSocket(net::SocketDescriptor sd) {
    this->Close(false);
    this->sd = sd;
    this->state = SocketState::CONNECTED;
}

void TcpSocket::Push(const Packet &pkt) {
    assert(pkt.position > sizeof(Packet_Header));
    assert((reinterpret_cast<const Packet_Header *>(&pkt.buffer[0]))->size == pkt.position);
    this->send.queue.emplace_back(pkt.buffer);
}

void TcpSocket::Push(Packet &&pkt) {
    assert(pkt.position > sizeof(Packet_Header));
    assert((reinterpret_cast<const Packet_Header *>(&pkt.buffer[0]))->size == pkt.position);
    this->send.queue.emplace_back(ToRvalue(pkt.buffer));
}

bool TcpSocket::Pop(Packet &out) {
    if (this->recv.queue.empty()) {
        return false;
    }

    out.Reset(ToRvalue(this->recv.queue.front()));
    this->recv.queue.pop_front();

    return true;
}

SocketResult TcpSocket::DoConnect() {
    if (this->state == SocketState::CONNECTED) {
        return SocketResult::DONE;
    }

    pollfd pfd{.fd = this->sd, .events = POLLOUT};

#ifdef WINDOWS
    WSAPoll(&pfd, 1, 0);
#else
    poll(&pfd, 1, 0);
#endif

    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        LogError("socket", "Connect poll error");
        this->Close(true);
        return SocketResult::ERROR;
    }

    if (pfd.revents & POLLOUT) {
        if (net::HasSocketError(this->sd)) {
            LogError("socket", "Connect error: {}"_format(net::GetErrorString()));
            this->Close(true);
            return SocketResult::ERROR;
        }

        this->state = SocketState::CONNECTED;
        return SocketResult::DONE;
    }

    return SocketResult::NOT_DONE;
}

SocketResult TcpSocket::DoSend() {
    if (this->state != SocketState::CONNECTED) {
        return SocketResult::DONE;
    }

    if (net::HasSocketError(this->sd)) {
        this->Close(true);
    }

    while (true) {
        if (this->state != SocketState::CONNECTED) {
            return SocketResult::DONE;
        }

        if (this->send.current.empty() && !this->send.queue.empty()) {
            this->send.current = ToRvalue(this->send.queue.front());
            this->send.queue.pop_front();
            this->send.pos = 0;
        }

        if (this->send.current.empty()) {
            return SocketResult::DONE;
        }

        assert(this->send.pos < this->send.current.size());

        auto bytesleft = static_cast<int>(this->send.current.size() - this->send.pos);
        if (bytesleft == 0) {
            return SocketResult::DONE;
        }

        auto sent = ::send(this->sd, &this->send.current[this->send.pos], bytesleft, 0);

        if (sent == -1) {
            if (!net::IsEWouldBlock()) {
                LogError("socket", "send() error: {}"_format(net::GetErrorString()));
                this->Close(true);
                return SocketResult::ERROR;
            }

            return SocketResult::NOT_DONE;
        }

        this->send.pos += sent;
        this->stats.bytes_sent += sent;
        TcpSocket::global_stats.bytes_sent += sent;

        if (this->send.pos != this->send.current.size()) {
            return SocketResult::NOT_DONE;
        }

        this->send.current.clear();
        ++this->stats.packets_sent;
        ++TcpSocket::global_stats.packets_sent;
    }
}

SocketResult TcpSocket::DoRecv() {
    if (this->state != SocketState::CONNECTED) {
        return SocketResult::DONE;
    }

    if (net::HasSocketError(this->sd)) {
        this->Close(true);
    }

    while (true) {
        if (this->state != SocketState::CONNECTED) {
            return SocketResult::DONE;
        }

        if (this->recv.current.size() < sizeof(Packet_Header)) {
            this->recv.current.resize(sizeof(Packet_Header));
            this->recv.pos = 0;
        }

        assert(this->recv.pos < this->recv.current.size());

        auto bytesleft = static_cast<int>(this->recv.current.size() - this->recv.pos);
        auto received = ::recv(this->sd, &this->recv.current[this->recv.pos], bytesleft, 0);

        if (received == -1) {
            if (!net::IsEWouldBlock()) {
                LogError("socket", "recv() error: {}"_format(net::GetErrorString()));
                this->Close(true);
                return SocketResult::ERROR;
            }

            return SocketResult::NOT_DONE;
        } else if (received == 0) {
            this->Close(true);
            return SocketResult::ERROR;
        }

        this->recv.pos += received;
        this->stats.bytes_received += received;
        TcpSocket::global_stats.bytes_received += received;

        if (this->recv.pos != this->recv.current.size()) {
            return SocketResult::NOT_DONE;
        }

        if (this->recv.current.size() == sizeof(Packet_Header)) {
            Packet_Header hdr;
            memcpy(&hdr, this->recv.current.data(), sizeof(Packet_Header));

            if (hdr.size < sizeof(Packet_Header) || hdr.size > max_packet_size) {
                assert(!"Invalid packet size");
            }

            this->recv.current.resize(hdr.size);
            //memcpy(this->recv.current.data(), &hdr, sizeof(packet_hdr));
        } else {
            this->recv.queue.emplace_back(ToRvalue(this->recv.current));
            this->recv.current.clear();
            ++this->stats.packets_received;
            ++TcpSocket::global_stats.packets_received;
        }
    }
}
