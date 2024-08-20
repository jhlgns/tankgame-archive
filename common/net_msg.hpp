#pragma once

#include "common/common.hpp"
#include "common/session_info.hpp"
#include "common/packet.hpp"
#include "common/player_info.hpp"
#include "common/disconnect_reason.hpp"

#include <variant>

enum class NetMessageType : u8 {
    HANDSHAKE            = 1,
    PING                 = 2,
    PONG                 = 3,
    GET_SESSION_INFO     = 4,
    CREATE_SESSION       = 5,
    JOIN_SESSION         = 6,
    LEAVE_SESSION        = 7,
    READY                = 8,
    GAME_STARTED         = 9,
    LOAD_LEVEL           = 10,
    GAME_COMMAND         = 11,
    SHUTDOWN             = 12,
    SET_TICK_LENGTH      = 13,
    PAUSE_GAME           = 14,
    LOBBY_UPDATE         = 15,
    DISCONNECT           = 16,
    COUNT
};

template<NetMessageType TheType>
struct NetMessage {
    constexpr static NetMessageType Type = TheType;

    void Serialize(Packet &packet) const {
        packet.WriteEnum(Type);
    }
};

struct HandshakeRequest : public NetMessage<NetMessageType::HANDSHAKE> {
    u16 ver_major;
    u16 ver_minor;
    u16 ver_build;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->ver_major);
        packet.WriteU16(this->ver_minor);
        packet.WriteU16(this->ver_build);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU16(this->ver_major) &&
            packet.ReadU16(this->ver_minor) &&
            packet.ReadU16(this->ver_build);
    }
};

struct HandshakeResponse : public NetMessage<NetMessageType::HANDSHAKE> {
    u16 ver_major;
    u16 ver_minor;
    u16 ver_build;
    bool ok;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->ver_major);
        packet.WriteU16(this->ver_minor);
        packet.WriteU16(this->ver_build);
        packet.WriteB8(this->ok);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU16(this->ver_major) &&
            packet.ReadU16(this->ver_minor) &&
            packet.ReadU16(this->ver_build) &&
            packet.ReadB8(this->ok);
    }
};

struct PingMessage : public NetMessage<NetMessageType::PING> {
    f32 my_time = -123.45f;

    inline void Serialize(Packet& packet) const {
        NetMessage::Serialize(packet);

        packet.WriteF32(this->my_time);
    }

    inline bool Deserialize(Packet& packet) {
        return packet.ReadF32(this->my_time);
    }
};

struct PongMessage : public NetMessage<NetMessageType::PONG> {
    f32 my_time = -123.45f; // The recipient time at the arrival of the corresponding ping message (The client's time when receiving the ping)
    f32 your_time = -123.45f; // The time that was sent with the ping message (The server's time when sending the ping)

    inline void Serialize(Packet& packet) const {
        NetMessage::Serialize(packet);

        packet.WriteF32(this->my_time);
        packet.WriteF32(this->your_time);
    }

    inline bool Deserialize(Packet& packet) {
        return
            packet.ReadF32(this->my_time) &&
            packet.ReadF32(this->your_time);
    }
};

struct GetSessionInfoRequest : public NetMessage<NetMessageType::GET_SESSION_INFO> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct GetSessionInfoResponse : public NetMessage<NetMessageType::GET_SESSION_INFO> {
    Array<SessionInfo> sessions;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->sessions.size());

        for (const auto &info : this->sessions) {
            packet.WriteString(info.name);
            packet.WriteU16(info.id);
            packet.WriteU16(info.nplayers);
            packet.WriteU16(info.nplayers_connected);
            packet.WriteEnum(info.state);
            packet.WriteB8(info.haspw);
        }
    }

    inline bool Deserialize(Packet &packet) {
        u16 num_sessions;
        if (!packet.ReadU16(num_sessions)) {
            return false;
        }

        this->sessions.resize(num_sessions);

        for (auto &info : this->sessions) {
            if (!packet.ReadString(info.name) ||
                !packet.ReadU16(info.id) ||
                !packet.ReadU16(info.nplayers) ||
                !packet.ReadU16(info.nplayers_connected) ||
                !packet.ReadEnum(info.state) ||
                !packet.ReadB8(info.haspw)) {
                return false;
            }
        }

        return true;
    }
};

struct CreateSessionRequest : public NetMessage<NetMessageType::CREATE_SESSION> {
    u16 num_players;
    u16 num_bots;
    String name;
    String password;
    String player_name;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->num_players);
        packet.WriteU16(this->num_bots);
        packet.WriteString(this->name);
        packet.WriteString(this->password);
        packet.WriteString(this->player_name);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU16(this->num_players) &&
            packet.ReadU16(this->num_bots) &&
            packet.ReadString(this->name) &&
            packet.ReadString(this->password) &&
            packet.ReadString(this->player_name);
    }
};

struct CreateSessionResponse : public NetMessage<NetMessageType::CREATE_SESSION> {
    u16 created_session_id;
    bool success;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->created_session_id);
        packet.WriteB8(this->success);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU16(this->created_session_id) &&
            packet.ReadB8(this->success);
    }
};

struct JoinSessionRequest : public NetMessage<NetMessageType::JOIN_SESSION> {
    u16 session_id;
    String player_name;
    String password;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU16(this->session_id);
        packet.WriteString(this->player_name);
        packet.WriteString(this->password);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU16(this->session_id) &&
            packet.ReadString(this->player_name) &&
            packet.ReadString(this->password);
    }
};

struct JoinSessionResponse : public NetMessage<NetMessageType::JOIN_SESSION> {
    JoinSessionResult result;
    Array<PlayerInfo> connected_players;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteEnum(this->result);
        packet.WriteU16(this->connected_players.size());

        for (const auto &player_info : this->connected_players) {
            player_info.Serialize(packet);
        }
    }

    inline bool Deserialize(Packet &packet) {
        if (!packet.ReadEnum(this->result)) {
            return false;
        }

        u16 num_connected_players;
        packet.ReadU16(num_connected_players);
        this->connected_players.resize(num_connected_players);

        for (size_t i = 0; i < num_connected_players; ++i) {
            this->connected_players[i].Deserialize(packet);
        }

        return true;
    }
};

struct LeaveSessionMessage : public NetMessage<NetMessageType::LEAVE_SESSION> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct ReadyMessage : public NetMessage<NetMessageType::READY> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct GameStartedMessage : public NetMessage<NetMessageType::GAME_STARTED> {
    u32 player_tank = 0xDeadBeef;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteU32(this->player_tank);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadU32(this->player_tank);
    }
};

struct LoadLevelMessage : public NetMessage<NetMessageType::LOAD_LEVEL> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct GameCommandMessage : public NetMessage<NetMessageType::GAME_COMMAND> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct ShutdownMessage : public NetMessage<NetMessageType::SHUTDOWN> {
    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);
    }

    inline bool Deserialize(Packet &packet) {
        return true;
    }
};

struct SetTickLengthMessage: public NetMessage<NetMessageType::SET_TICK_LENGTH> {
    i16 tick_length_delta_microseconds = 1234;
    u16 duration_milliseconds = 1234;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteI16(this->tick_length_delta_microseconds);
        packet.WriteU16(this->duration_milliseconds);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadI16(this->tick_length_delta_microseconds) &&
            packet.ReadU16(this->duration_milliseconds);
    }
};

struct PauseGameMessage : public NetMessage<NetMessageType::PAUSE_GAME> {
    bool paused;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteB8(this->paused);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadB8(this->paused);
    }
};

struct LobbyUpdateMessage : public NetMessage<NetMessageType::LOBBY_UPDATE> {
    enum class UpdateType { // Only for (de)serialization
        PLAYER_JOINED,
        PLAYER_LEFT,
        UPDATE_PLAYER_INFO,
        UPDATE_PING,
    };

    struct PlayerJoined {
        PlayerInfo player_info;
    };

    struct PlayerLeft {
        String player_name;
    };

    struct UpdatePlayerInfo {
        PlayerInfo player_info;
    };

    std::variant<
        PlayerJoined,
        PlayerLeft,
        UpdatePlayerInfo>
        data;

    bool player_left = false;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        std::visit(
            Overloaded{
                [&](const PlayerJoined &player_joined) {
                    packet.WriteEnum(UpdateType::PLAYER_JOINED);
                    player_joined.player_info.Serialize(packet);
                },
                [&](const PlayerLeft &player_left) {
                    packet.WriteEnum(UpdateType::PLAYER_LEFT);
                    packet.WriteString(player_left.player_name);
                },
                [&](const UpdatePlayerInfo &update_player_info) {
                    packet.WriteEnum(UpdateType::UPDATE_PLAYER_INFO);
                    update_player_info.player_info.Serialize(packet);
                },
            },
            this->data);
    }

    inline bool Deserialize(Packet &packet) {
        UpdateType type;

        if (!packet.ReadEnum(type)) {
            return false;
        }

        switch (type) {
            case UpdateType::PLAYER_JOINED: {
                PlayerJoined player_joined;
                if (player_joined.player_info.Deserialize(packet)) {
                    this->data.emplace<PlayerJoined>(ToRvalue(player_joined));
                    return true;
                }
            } return false;

            case UpdateType::PLAYER_LEFT: {
                PlayerLeft player_left;
                if (packet.ReadString(player_left.player_name)) {
                    this->data.emplace<PlayerLeft>(ToRvalue(player_left));
                    return true;
                }
            } return false;

            case UpdateType::UPDATE_PLAYER_INFO: {
                UpdatePlayerInfo update_player_info;
                if (update_player_info.player_info.Deserialize(packet)) {
                    this->data.emplace<UpdatePlayerInfo>(ToRvalue(update_player_info));
                    return true;
                }
            } return false;

            default:
            case UpdateType::UPDATE_PING: {
                assert(false);
            }
        }
    }
};

struct DisconnectMessage : public NetMessage<NetMessageType::DISCONNECT> {
    DisconnectReason reason = DisconnectReason::NONE;
    String message;

    inline void Serialize(Packet &packet) const {
        NetMessage::Serialize(packet);

        packet.WriteEnum(this->reason);
        packet.WriteString(this->message);
    }

    inline bool Deserialize(Packet &packet) {
        return
            packet.ReadEnum(this->reason) &&
            packet.ReadString(this->message);
    }
};
