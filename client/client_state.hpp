#pragma once

#include "common/net_msg.hpp"
#include "common/common.hpp"
#include "common/net_msg_handler_map.hpp"
#include "common/entity.hpp"

union SDL_Event;
struct Client;
struct Packet;
struct PlayerInfo;

struct ClientState {
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void Input(const SDL_Event &event) = 0;
    virtual void Tick(f32 dt) = 0;
    virtual void Render() = 0;
    virtual StringView GetDisplayName() const = 0;

    NetMessageHandlerMap net_message_handlers;
};

namespace client_states {

UniquePtr<ClientState> MakeMenu();
UniquePtr<ClientState> MakeOptions();
UniquePtr<ClientState> MakeConnecting(Optional<StringView> ip = std::nullopt);
UniquePtr<ClientState> MakeHandshake();
UniquePtr<ClientState> MakeSessionbrowser(bool show_error_message = false);
UniquePtr<ClientState> MakeCreateSession();
UniquePtr<ClientState> MakeLobby(Array<PlayerInfo> connected_players);
UniquePtr<ClientState> MakeIngame(Entity my_tank);

}
