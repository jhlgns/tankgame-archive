#include "server/session.hpp"

#include "server/server_game_state.hpp"
#include "server/server.hpp"
#include "server/client_connection_state.hpp"
#include "common/log.hpp"
#include "common/player_info.hpp"

Session::Session(Server *server)
    : server(server) {
}

Session::~Session() = default;

void Session::Start(i32 id, StringView name, StringView pw, i32 nplayers, i32 num_npcs, bool persistent) {
    assert(nplayers >= 1);
    this->id = id;
    this->name = name;
    this->password = pw;
    this->num_players = nplayers;
    this->num_npcs = num_npcs;
    this->is_persistent = persistent;
    this->state = SessionState::LOBBY;
}

JoinSessionResult Session::Join(ClientConnection &con, StringView player_name, StringView password) {
    if (this->state != SessionState::LOBBY) {
        return JoinSessionResult::INVALID_STATE;
    }

    if (con.session_id.has_value()) {
        return JoinSessionResult::ALREADY_CONNECTED;
    }

    if (this->GetNumberOfConnectedPlayers() == this->num_players) {
        return JoinSessionResult::SESSION_FULL;
    }

    if (this->password != password) {
        return JoinSessionResult::WRONG_PASSWORD;
    }

    i32 name_collision_index = 0;

    for (const auto &player : this->players) {
        if (player.has_value()) {
            if (player.value().name == player_name) {
#if defined(DEVELOPMENT) && DEVELOPMENT
                ++name_collision_index;
#else
                return Join_Session_Result::NAME_TAKEN;
#endif
            }
        }
    }

    con.player_id = this->players.size();
    auto &player = this->players.emplace_back(&con);
    player->name = player_name;
#if defined(DEVELOPMENT) && DEVELOPMENT
    player->name_collision_index = name_collision_index;
#endif
    con.session_id = this->id;

    LobbyUpdateMessage update_message;
    update_message.data.emplace<LobbyUpdateMessage::PlayerJoined>(
        LobbyUpdateMessage::PlayerJoined{
            .player_info = this->GetPlayerInfo(player.value())
        });
    this->Broadcast(update_message);

    return JoinSessionResult::SUCCESS;
}

bool Session::Remove(ClientConnection &con) {
    if (!this->HasPlayer(con)) {
        return false;
    }

    if (!con.player_id.has_value()) {
        return false;
    }

    auto &player = this->players[con.player_id.value()];

    if (!player.has_value()) {
        return false;
    }

    LobbyUpdateMessage update_message;
    update_message.data.emplace<LobbyUpdateMessage::PlayerLeft>(
        LobbyUpdateMessage::PlayerLeft{
            .player_name = player.value().name
        });
    this->Broadcast(update_message);

    player.reset();
    con.session_id.reset();
    con.player_id.reset();

    if (this->GetNumberOfConnectedPlayers() == 0) {
        this->game_state.reset();
        this->players.clear();

        if (this->is_persistent) {
            LogInfo("session", "Restarting persistent session {}"_format(this->id));
            this->state = SessionState::LOBBY;
        } else {
            LogInfo("session", "Session {} ended"_format(this->id));
            this->state = SessionState::GARBAGE;
        }
    }

    return true;
}

bool Session::HasPlayer(const ClientConnection &con) const {
    assert(!(con.player_id.has_value() && static_cast<size_t>(con.player_id.value()) < this->players.size()) ||
        this->players.at(con.player_id.value()).value().con == &con);

    return
        con.player_id.has_value() &&
        static_cast<size_t>(con.player_id.value()) < this->players.size() &&
        this->players.at(con.player_id.value()).value().con == &con;
}

bool Session::SetPlayerReady(ClientConnection &con) {
    if (this->state != SessionState::LOBBY) {
        LogWarning("session", "Player {} ready request in non-lobby state"_format(con.player_id.value()));
        return false;
    }

    auto &player = this->players.at(con.player_id.value());

    if (!player.has_value()/* || player.value().ready*/) {
        LogWarning("session", "Player {} ready request multiple times"_format(con.player_id.value()));
        return false;
    }

    //player.value().ready = true;
    player.value().ready = !player.value().ready;
    LogInfo("session", "Player {} ready ({}/{})"_format(
        con.player_id.value(),
        this->GetNumberOfConnectedPlayers(true),
        this->num_players));

    // Notify other clients
    LobbyUpdateMessage lobby_update;
    lobby_update.data.emplace<LobbyUpdateMessage::UpdatePlayerInfo>(
        LobbyUpdateMessage::UpdatePlayerInfo{
            .player_info = this->GetPlayerInfo(player.value())
        });
    Packet lobby_update_packet;
    lobby_update.Serialize(lobby_update_packet);
    this->BroadcastPacket(ToRvalue(lobby_update_packet));

    // Check if we should start the game
    auto all_players_ready = this->GetNumberOfConnectedPlayers(true) == this->num_players;

    if (all_players_ready) {
        LogInfo("session", "All players ready, starting game");
        this->StartGame();
    }

    return true;
}

void Session::StartGame() {
    this->game_state = std::make_unique<ServerGameState>(this);
    this->game_state->Prepare();

    for (auto& player : this->players) {
        if (!player.has_value()) {
            continue;
        }

        assert(this->game_state->entities.IsValid(player.value().tank_id));

        GameStartedMessage message;
        message.player_tank = entt::to_integral(player.value().tank_id);
        player.value().con->Send(message);
    }

    this->state = SessionState::INGAME;

    LoadLevelMessage message;
    Packet level_packet;
    message.Serialize(level_packet);
    this->game_state->Serialize(level_packet);
    level_packet.WriteHeader();

    for (auto &player : this->players) {
        if (player.has_value()) {
            auto &con = player.value().con;
            con->SendPacketCopy(level_packet);
            con->SetNextState(client_connection_states::MakeIngame(con));
        }
    }
}

SessionPlayer &Session::GetPlayer(ClientConnection &con) {
    CHECK(this->HasPlayer(con));
    CHECK(con.player_id.has_value());

    auto &player = this->players.at(con.player_id.value());
    CHECK(player.has_value());

    return player.value();
}

void Session::Tick(f32 dt) {
    if (this->state != SessionState::INGAME || this->game_state == nullptr) {
        return;
    }

    this->game_state->Tick(dt);
}

void Session::BroadcastPacket(Packet &&packet) {
    packet.WriteHeader();

    for (const auto &player : this->players) {
        if (player.has_value()) {
            player.value().con->SendPacketCopy(packet);
        }
    }
}

i32 Session::GetNumberOfConnectedPlayers(bool only_ready) const {
    if (only_ready) {
        return std::count_if(this->players.begin(), this->players.end(),
            [](const auto &player) {
                return player.has_value() && player.value().ready;
            });
    } else {
        return std::count_if(this->players.begin(), this->players.end(),
            [](const auto &player) {
                return player.has_value();
            });
    }
}

PlayerInfo Session::GetPlayerInfo(const SessionPlayer &player) const {
    return PlayerInfo{
        .name = player.name,
        .display_name = player.GetDisplayName(),
        .ready = player.ready
    };
}
