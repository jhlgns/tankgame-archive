#include "common/net_msg_handler_map.hpp"

#include "common/log.hpp"

bool NetMessageHandlerMap::HandlePacket(Packet &&client_packet) {
    NetMessageType type;
    if (!client_packet.ReadEnum(type)) {
        return false;
    }

    auto it = this->net_message_handlers.find(type);
    if (it == this->net_message_handlers.end()) {
        return false;
    }

    auto got_exception = false;

    try {
        it->second(ToRvalue(client_packet));
    } catch (const std::exception &ex) {
        got_exception = true;
        LogInfo("Net_Message_Handler_Map", "Got exception while invoking message callback: {}"_format(ex.what()));
    }

    return !got_exception && client_packet.IsValidAndFinished();
}
