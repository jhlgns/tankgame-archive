#pragma once

#include "common.hpp"

struct Server;
struct GameState;
struct ClientConnection;

enum class JoinSessionResult {
    SUCCESS               = 0,
    NOT_FOUND             = 1,
    INVALID_STATE         = 2,
    ALREADY_CONNECTED     = 3,
    SESSION_FULL          = 4,
    WRONG_PASSWORD        = 5,
    NAME_TAKEN            = 6,
};

inline String ToString(JoinSessionResult result) {
    switch (result) {
        case JoinSessionResult::SUCCESS:
            return "Success";
        case JoinSessionResult::NOT_FOUND:
            return "Session not found";
        case JoinSessionResult::INVALID_STATE:
            return "Invalid state";
        case JoinSessionResult::ALREADY_CONNECTED:
            return "Already connected";
        case JoinSessionResult::SESSION_FULL:
            return "Session is full";
        case JoinSessionResult::WRONG_PASSWORD:
            return "Wrong password";
        case JoinSessionResult::NAME_TAKEN:
            return "Name taken";
        default:
            return "(unknown)";
    }
}

enum class SessionState {
    LOBBY,
    INGAME,
    RESULT,
    GARBAGE,
};

struct SessionInfo {
    String name;
    u16 id;
    u16 nplayers;
    u16 nplayers_connected;
    SessionState state;
    b8 haspw;
};
