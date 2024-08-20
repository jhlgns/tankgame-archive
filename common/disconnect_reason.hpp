#pragma once

enum class DisconnectReason {
    NONE,
    ERROR,
    INVALID,
    PROTO_ERR,
    KICK,
};

inline String ToString(DisconnectReason reason) {
    switch (reason) {
        case DisconnectReason::ERROR:     return "An error occurred";
        case DisconnectReason::INVALID:   return "Invalid parameter";
        case DisconnectReason::PROTO_ERR: return "Protocol error";
        case DisconnectReason::KICK:      return "You were kicked";
        case DisconnectReason::NONE:
        default:                           return "(unknown)";
    }
}
