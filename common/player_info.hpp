#pragma once

#include "common/packet.hpp"

struct PlayerInfo {
    String name;
    String display_name;
    bool ready = false;

    void Serialize(Packet &packet) const {
        packet.WriteString(this->name);
        packet.WriteString(this->display_name);
        packet.WriteB8(this->ready);
    }

    bool Deserialize(Packet &packet) {
        return
            packet.ReadString(this->name) &&
            packet.ReadString(this->display_name) &&
            packet.ReadB8(this->ready);
    }
};
