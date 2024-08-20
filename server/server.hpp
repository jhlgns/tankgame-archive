#pragma once

#include "common/common.hpp"
#include "common/packet.hpp"
#include "server/session.hpp"
#include "common/net_msg.hpp"
#include "common/socket.hpp"
#include "server/client_connection.hpp"

struct Server {
    Server();
    ~Server();
    bool Start();
    void Quit();
    void MainLoop();
    void Tick();
    void NotifySent(ClientConnection &con);
    Optional<i32> CreateSession(StringView name, StringView password, i32 num_players, i32 num_npcs, bool persistent);
    Session *TryGetSession(i32 id);
    ClientConnection *TryGetConnection(i32 id);
    void GetInfo(GetSessionInfoResponse &output) const;
    void DoAccept();

    inline void	ProtoErr(ClientConnection &con) {
        con.Close(false, DisconnectReason::PROTO_ERR, "Protocol error");
    }

    constexpr static i32 default_port = 1303;
    net::SocketDescriptor sd = -1;
    Array<pollfd> pollfds;
    Array<UniquePtr<ClientConnection>> clients;
    Array<UniquePtr<Session>> sessions;
    bool quit_flag = false;
};

Server &GetServer();
