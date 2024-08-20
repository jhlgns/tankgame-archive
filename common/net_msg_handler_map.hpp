#pragma once

#include "common/common.hpp"
#include "common/packet.hpp"
#include "common/net_msg.hpp"

struct NetMessageHandlerMap {
    bool HandlePacket(Packet &&client_packet);

    template<typename Arg, typename That>
    void Add(void (That::*callback)(Arg &&), That *that) {
        this->net_message_handlers.emplace(
            Arg::Type,
            [callback, that](Packet &&packet) {
                Arg arg;

                if (!arg.Deserialize(packet)) {
                    assert(false); // TODO @Security @Robustness
                }

                (that->*callback)(ToRvalue(arg));
            });
    }

    template<NetMessageType Type, typename That>
    void Add(void (That::*callback)(Packet &&), That *that) {
        this->net_message_handlers.emplace(
            Type,
            [callback, that](Packet &&packet) {
                (that->*callback)(ToRvalue(packet));
            });
    }

    HashMap<NetMessageType, std::function<void(Packet &&)>> net_message_handlers;
};
