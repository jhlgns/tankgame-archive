#pragma once

#include "packet.hpp"
#include "net_platform.hpp"

#include <queue>
#include <memory>

enum class SocketState {
    NONE,
    CONNECTING,
    CONNECTED,
    ERROR,
};

#undef ERROR
enum class SocketResult {
    NOT_DONE,
    DONE,
    ERROR,
};

struct SocketStats {
    size_t bytes_sent = 0;
    size_t packets_sent = 0;
    size_t bytes_received = 0;
    size_t packets_received = 0;
    size_t num_connections = 0;
};

struct SocketBuffer {
    inline void Reset() {
        this->queue.clear();
        this->current.clear();
        this->pos = 0;
    }

    std::deque<Array<char>> queue;
    Array<char> current;
    size_t pos = 0;
};

struct TcpSocket {
    ~TcpSocket();
    TcpSocket() = default;

    inline TcpSocket(TcpSocket &&other) noexcept {
        this->SetConnectedSocket(other.sd);
        other.sd = -1;
        other.state = SocketState::NONE;
        other.remote_address = {};
        other.send = {};
        other.send = {};
    }

    TcpSocket &operator=(TcpSocket &&other) noexcept {
        if (this != &other) {
            this->SetConnectedSocket(other.sd);
            other.sd = -1;
            other.state = SocketState::NONE;
            other.remote_address = {};
            other.send = {};
            other.send = {};
        }

        return *this;
    }

    TcpSocket(const TcpSocket &) = delete;
    TcpSocket& operator=(const TcpSocket &) = delete;

    void Close(bool error);
    void Connect(sockaddr_in remote_address);
    void SetConnectedSocket(net::SocketDescriptor sd);
    void Push(const Packet &packet);
    void Push(Packet &&packet);
    bool Pop(Packet &out);
    SocketResult DoConnect();
    SocketResult DoSend();
    SocketResult DoRecv();

    static SocketStats global_stats;
    SocketStats stats;
    net::SocketDescriptor sd = -1;
    SocketState state = SocketState::NONE;
    sockaddr_in remote_address;
    SocketBuffer send;
    SocketBuffer recv;
};
