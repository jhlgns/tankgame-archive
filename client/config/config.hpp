#pragma once

#include "common/common.hpp"
#include "SDL.h"
#include "json.hpp"

struct Client;

struct Config {
    constexpr static i32 DEFAULT_DISPLAY_INDEX = 0;

    enum class WindowMode {
        FULLSCREEN,
        BORDERLESS,
        WINDOWED
    };

    void Serialize();
    void Deserialize();
    void Save();
    void Load();
    void ApplyWindowSettings();
    SDL_DisplayMode GetDisplayMode() const;

    struct {
        int display_mode_index;
        WindowMode window_mode;
        bool limit_fps = false;
        String player_name;
        String assets_dir;
        bool force_localhost = false;
    } values;

    nlohmann::json storage;
};

Config &GetConfig();

