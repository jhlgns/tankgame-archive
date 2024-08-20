#include "server/server.hpp"

#include "common/packet.hpp"
#include "common/net_msg.hpp"
#include "common/game_state.hpp"
#include "common/net_platform.hpp"
#include "common/log.hpp"
#include "common/frame_timer.hpp"

Server::Server() = default;
Server::~Server() = default;

bool Server::Start() {
    LogInfo("server", "Starting the server");

    this->sd = net::CreateNonBlockingSocket();
    if (this->sd == -1) {
        LogError("server", "Unable to create socket");
        return false;
    }

    net::MakeReusable(this->sd);

    sockaddr_in svaddr;
    svaddr.sin_family = AF_INET;
    svaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svaddr.sin_port = htons(Server::default_port);

    if (::bind(this->sd, reinterpret_cast<sockaddr *>(&svaddr), sizeof(svaddr)) == -1) {
        LogError("server", "Unable to bind socket");
        return false;
    }

    if (::listen(this->sd, this->clients.size()) == -1) {
        LogError("server", "Unable to listen on socket");
        return false;
    }

    LogInfo("server", "Server running on port {}"_format(ntohs(svaddr.sin_port)));

    // The server is the first "client".
    // This means that there would be also a client_connection allocated in the Connections array which is not used.
    this->clients.emplace_back();
    this->pollfds.emplace_back(pollfd{.fd = this->sd, .events = POLLIN});

#if defined(DEVELOPMENT) && DEVELOPMENT
    this->CreateSession("developer",            {},      1,  1, true);
    this->CreateSession("Marcel D'avis",        {},      2,  4, true);
    /*this->CreateSession("Martin Sonneborn",     {},      2,  1, true);
    this->CreateSession("Donarudo Terampu",     "12345", 1,  0, false);
    this->CreateSession("Boris JSON",           "12345", 1,  0, false);
    this->CreateSession("Angular Merkel",       "12345", 99, 0, true);
    this->CreateSession("Weird corner casey",   "12345", 99, 0, true);
    this->CreateSession("Alpecin",              "12345", 1,  0, false);
    this->CreateSession("Schuppenshampoo",      "12345", 1,  0, false);
    this->CreateSession("Traegt dazu bei",      "12345", 1,  0, false);
    this->CreateSession("Die Wachstumsphasen",  "12345", 1,  0, false);
    this->CreateSession("Der Haarwurzeln",      "12345", 1,  0, false);
    this->CreateSession("Zu Verlaengern",       "12345", 1,  0, false);
    this->CreateSession("Dr. Klenk",            "12345", 2,  0, true);
    this->CreateSession("Bielefeld",            "12345", 2,  0, true);*/
#endif

    return true;
}

void Server::Quit() {
    LogInfo("server", "Setting quit flag");

    if (this->quit_flag) {
        LogWarning("server", "Client already quit");
        return;
    }

    this->quit_flag = true;
}

void Server::MainLoop() {
    LogInfo("Server", "Starting the main loop");

    auto &timer = GetFrameTimer();
    timer.Start();

    while (!this->quit_flag) {
        timer.BeginFrame();

        u32 ticks_done = 0;
        while (!timer.FrameDone()) {
            timer.BeginTick();
            this->Tick();
            timer.AdvanceTick();

            if (++ticks_done > 100) {
                LogWarning("server", "Cannot keep up the framerate! Did {} ticks in this main loop iteration"_format(ticks_done));
            }
        }
    }

    LogInfo("Server", "Main loop exit");
}

void Server::Tick() {
#if 0
    for (const auto &connection : this->clients) {
        if (connection != nullptr) {
            log_info(
                "socket stats client {}"_format(connection->id),
                 "{} bytes ({} packets)"_format(
                    connection->socket.stats.bytes_sent, connection->socket.stats.packets_sent));
        }
    }
#endif

    net::Poll(&this->pollfds[0], this->pollfds.size(), 0);

    assert(!(this->pollfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)));
    if (this->pollfds[0].revents & POLLIN) {
        this->DoAccept();
    }

    auto dt = GetFrameTimer().dt;

    for (i32 client_id = 1; client_id < static_cast<i32>(this->clients.size()); ++client_id) {
        auto &con = this->clients[client_id];
        if (con == nullptr) {
            continue;
        }

        auto &fd = this->pollfds[client_id];

        if (con->garbage) {
            fd.fd = -1;
            fd.events = 0;
            con.reset();
        } else {
            if (fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (!con->closed) {
                    con->Close(false, DisconnectReason::ERROR, "poll error");
                }

                continue;
            }

            auto incoming = (fd.revents & POLLIN) != 0;
            auto outgoing = (fd.revents & POLLOUT) != 0;
            con->Tick(dt, incoming, outgoing);
        }
    }

    for (auto &session : this->sessions) {
        if (session != nullptr) {
            if (session->state == SessionState::GARBAGE) {
                LogInfo("server", "Removing garbage session {}"_format(session->id));
                session.reset();
            } else {
                session->Tick(dt);
            }
        }
    }
}

void Server::NotifySent(ClientConnection &con) {
    assert(static_cast<size_t>(con.id) < this->pollfds.size());
    this->pollfds[con.id].events |= POLLOUT;
}

Optional<i32> Server::CreateSession(StringView name, StringView password, i32 num_players, i32 num_npcs, bool persistent) {
    if (name.empty() || name.size() > 20) {
        LogWarning("server", "Cannot create session, invalid name");
        return std::nullopt;
    }

    if (password.size() > 20) {
        LogWarning("server", "Cannot create session, invalid name");
        return std::nullopt;
    }

    if (num_players < 1 || num_players > 100) {
        LogWarning("server", "Cannot create session {}, invalid number of players ({})"_format(name, num_players));
        return std::nullopt;
    }

    LogInfo("server", "Creating session {}, password: {}, number of players: {}"_format(name, password, num_players));

    i32 session_id = 0;
    auto free_found = false;

    for (; session_id < static_cast<i32>(this->sessions.size()); ++session_id) {
        auto &maybe = this->sessions[session_id];

        if (maybe == nullptr) {
            free_found = true;
            maybe = std::make_unique<Session>(this);
            break;
        }
    }

    if (!free_found) {
        session_id = this->sessions.size();
        this->sessions.emplace_back(std::make_unique<Session>(this));
    }

    this->sessions[session_id]->Start(session_id, name, password, num_players, num_npcs, persistent);

    return session_id;
}

Session *Server::TryGetSession(i32 id) {
    if (static_cast<size_t>(id) >= this->sessions.size()) {
        return nullptr;
    }

    return this->sessions.at(id).get();
}

ClientConnection *Server::TryGetConnection(i32 id) {
    if (static_cast<size_t>(id) >= this->clients.size()) {
        return nullptr;
    }

    return this->clients.at(id).get();
}

void Server::GetInfo(GetSessionInfoResponse &output) const {
    size_t num_sessions = 0;
    for (const auto &session : this->sessions) {
        if (session != nullptr) {
            ++num_sessions;
        }
    }

    output.sessions.resize(num_sessions);

    size_t info_index = 0;
    for (size_t i = 0; i < this->sessions.size(); ++i) {
        const auto &session = this->sessions[i];

        if (session != nullptr) {
            auto &info = output.sessions[info_index];
            info.name = session->name;
            info.id = i;
            info.nplayers = session->num_players;
            info.nplayers_connected = session->GetNumberOfConnectedPlayers();
            info.state = session->state;
            info.haspw = !session->password.empty();
            ++info_index;
        }
    }
}

void Server::DoAccept() {
    sockaddr_in client_address;
    auto client_socket = net::AcceptNonBlockingSocket(this->sd, &client_address);

    if (client_socket == -1) {
        if (net::IsEWouldBlock()) {
            return;
        }

        LogWarning("server", "Failed to accept client");
        return;
    }

    i32 client_id = 1;
    auto free_found = false;

    for (; client_id < static_cast<i32>(this->clients.size()); ++client_id) {
        if (this->clients[client_id] == nullptr) {
            free_found = true;
            break;
        }
    }

    if (!free_found) {
        client_id = static_cast<i32>(this->clients.size());
        this->clients.emplace_back();
        this->pollfds.emplace_back();
    }

    LogInfo("server", "Client connected: {}:{}"_format(inet_ntoa(client_address.sin_addr), client_address.sin_port));

    auto &con = this->clients[client_id];
    assert(con == nullptr);
    TcpSocket tcp_socket;
    tcp_socket.SetConnectedSocket(client_socket);
    con = std::make_unique<ClientConnection>(client_id, ToRvalue(tcp_socket));
    con->Start();

    this->pollfds[client_id] = {.fd = client_socket, .events = POLLIN };
}

Server &GetServer() {
    static Server res;
    return res;
}

