#pragma once

#include "common/common.hpp"
#include "common/packet.hpp"
#include "common/net_msg.hpp"
#include "common/socket.hpp"
#include "common/disconnect_reason.hpp"

struct ClientConnectionState;

struct ClientConnection {
    explicit ClientConnection(i32 id, TcpSocket &&socket);
    ~ClientConnection();
    void Start();
    void Close(bool force, DisconnectReason reason, StringView message);
    void Tick(f32 dt, bool incoming, bool outgoing);
    void SendPacket(Packet &&packet);
    void SendPacketCopy(const Packet &packet);
    void SetNextState(UniquePtr<ClientConnectionState> state);

    template<typename T>
    void Send(const T &data) {
        Packet packet;
        data.Serialize(packet);
        this->SendPacket(ToRvalue(packet));
    }

    inline bool IsAdmin() const {
        return true; // TODO @Release @NetSecurity
    }

    constexpr static chrono::high_resolution_clock::duration last_packet_timeout = 2s;

    i32 id;
    Optional<i32> session_id;
    Optional<i32> player_id;
    UniquePtr<ClientConnectionState> state;
    UniquePtr<ClientConnectionState> next_state;
    TcpSocket socket;
    bool garbage = false;
    bool closed = false;
    chrono::high_resolution_clock::time_point closed_at;
    std::array<f32, 32> time_diff_ringbuf{};
    std::size_t time_diff_ringbuf_pos = 0;
    std::array<f32, 32> rtt_ringbuf{};
    std::size_t rtt_ringbuf_pos = 0;
    f32 time_last_speed_change_requested = 0.0f;
};
