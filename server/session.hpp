#pragma once

#include "common/common.hpp"
#include "common/session_info.hpp"
#include "common/player_info.hpp"
#include "common/game_state.hpp"

struct Server;
struct Packet;
struct ServerGameState;
struct ClientConnection;

struct SessionPlayer {
    inline explicit SessionPlayer(ClientConnection *con)
        : con(con) {
    }

    String GetDisplayName() const {
#if defined(DEVELOPMENT) && DEVELOPMENT
        return "{} [dev:{}]"_format(this->name, this->name_collision_index);
#else
        return this->name;
#endif
    }

    ClientConnection *con;
    bool ready = false;
    Entity tank_id = entt::null;
    String name;
#if defined(DEVELOPMENT) && DEVELOPMENT
    i32 name_collision_index = 0;
#endif
};

struct Session {
    explicit Session(Server *server);
    ~Session();
    void Start(i32 id, StringView name, StringView password, i32 num_players, i32 num_npcs, bool persistent);
    JoinSessionResult Join(ClientConnection &con, StringView player_name, StringView password);
    bool Remove(ClientConnection &con);
    bool HasPlayer(const ClientConnection &con) const;
    bool SetPlayerReady(ClientConnection &con);
    void StartGame();
    SessionPlayer &GetPlayer(ClientConnection &con);
    void Tick(f32 dt);
    void BroadcastPacket(Packet &&packet);
    i32 GetNumberOfConnectedPlayers(bool only_ready = false) const;
    PlayerInfo GetPlayerInfo(const SessionPlayer &player) const;

    template<typename T>
    void Broadcast(const T &data) {
        Packet packet;
        data.Serialize(packet);
        this->BroadcastPacket(ToRvalue(packet));
    }

    Server *server;
    i32 id = -1;
    SessionState state;
    String name;
    String password;
    UniquePtr<ServerGameState> game_state;
    i32 num_players = 0;
    i32 num_npcs = 0;
    Array<Optional<SessionPlayer>> players;
    bool is_persistent = false;
};
