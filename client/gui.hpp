#pragma once

#include "common/common.hpp"
#include "common/player_info.hpp"

struct nk_context;
struct Client;
struct SessionInfo;
struct ClientGameState;
union SDL_Event;

struct MainMenu {
    void Show();
};

struct SessionBrowserMenu {
    void Show();

    Array<SessionInfo> *session_infos = nullptr;
    Array<std::array<char, 32>> password_buffers;
};

struct SessionLobbyMenu {
    void Show();

    Array<PlayerInfo> *player_info = nullptr;
};

struct CrateSessionMenu {
    void Show();

    char name[64];
    char password[64];
    i32 max_players = 1;
    i32 num_bots = 2;
};

struct OptionsMenu {
    void Show();

    int display_mode_index = 0;
    int window_mode = 0;
    int limit_fps = 0;
};

struct PauseMenu {
    void Show(ClientGameState &game_state);
};

struct GuiState {
    void Init();
    void BeginInput();
    void HandleEvent(SDL_Event *e);
    void EndInput();
    void Render();
    void ShowError();

    MainMenu           main_menu;
    SessionBrowserMenu session_browser_menu;
    SessionLobbyMenu   session_lobby_menu;
    CrateSessionMenu   create_session_menu;
    OptionsMenu        options_menu;
    PauseMenu          pause_menu;

    nk_context *ctx;
};
