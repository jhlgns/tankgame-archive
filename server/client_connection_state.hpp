#pragma once

#include "common/common.hpp"
#include "common/net_msg.hpp"
#include "common/net_msg_handler_map.hpp"

struct ClientConnection;

struct ClientConnectionState {
    inline explicit ClientConnectionState(ClientConnection *connection)
        : connection(connection) {
    }

    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void Tick(f32 dt) = 0;
    virtual StringView GetDisplayName() const = 0;

    ClientConnection *connection;
    NetMessageHandlerMap net_message_handlers;
};


namespace client_connection_states {

UniquePtr<ClientConnectionState> MakeHandshake(ClientConnection *connection);
UniquePtr<ClientConnectionState> MakeJoinSession(ClientConnection *connection);
UniquePtr<ClientConnectionState> MakeLobby(ClientConnection *connection);
UniquePtr<ClientConnectionState> MakeIngame(ClientConnection *connection);

}
