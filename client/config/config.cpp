#include "client/config/config.hpp"

#include "client/graphics/graphics_manager.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <SDL.h>

constexpr StringView file_path = "config.json";

inline bool operator ==(const SDL_DisplayMode& lhs, const SDL_DisplayMode& rhs) {
    return
        lhs.h == rhs.h &&
        lhs.w == rhs.w &&
        lhs.refresh_rate == rhs.refresh_rate;
}

static void SaveJsonFile(const nlohmann::json& json, StringView file_path) {
    std::ofstream ofs{file_path.data()};
    ofs << std::setw(4) << json << std::endl;
}

[[nodiscard]] static bool ReadJsonFile(nlohmann::json &json, StringView file_path) {
    std::ifstream ifs{file_path.data()};

    if (!ifs.is_open()) {
        return false;
    }

    ifs >> json;
    return !ifs.fail();
}

void to_json(nlohmann::json& json, const SDL_DisplayMode& mode) {
    json = {
        {"width", mode.w},
        {"height", mode.h},
        {"refresh_rate", mode.refresh_rate}
    };
}

void from_json(const nlohmann::json& json, SDL_DisplayMode& mode) {
    mode = SDL_DisplayMode{
        .w = json.at("width"),
        .h = json.at("height"),
        .refresh_rate = json.at("refresh_rate"),
    };
}

static Optional<int> FindDisplayModeIndex(const SDL_DisplayMode &mode) {
    SDL_DisplayMode closest_mode;
    SDL_GetClosestDisplayMode(0, &mode, &closest_mode);
    auto num_modes = SDL_GetNumDisplayModes(0);

    for (int i = 0; i < num_modes; i++) {
        SDL_DisplayMode current_mode;
        SDL_GetDisplayMode(Config::DEFAULT_DISPLAY_INDEX, i, &current_mode);

        if (current_mode == closest_mode) {
            return i;
        }
    }

    return std::nullopt;
}

void Config::Serialize() {
    this->storage["resolution"] = this->GetDisplayMode();
    this->storage["window_mode"] = static_cast<int>(this->values.window_mode);
    this->storage["limit_fps"] = this->values.limit_fps;
    this->storage["player_name"] = this->values.player_name;
    this->storage["assets_dir"] = this->values.assets_dir;
    this->storage["force_localhost"] = this->values.force_localhost;
}

void Config::Deserialize() {
    auto mode = this->storage.at("resolution").get<SDL_DisplayMode>();

    if (auto dmi = FindDisplayModeIndex(mode); dmi.has_value()) {
        this->values.display_mode_index = dmi.value();
    } else {
        FAIL("Could not find a suitable display mode");
    }

    this->values.window_mode = static_cast<WindowMode>(this->storage.at("window_mode").get<int>());
    this->values.limit_fps = this->storage.at("limit_fps");
    this->values.player_name = this->storage.at("player_name");
    this->values.assets_dir = this->storage.at("assets_dir");
    this->values.force_localhost = this->storage.at("force_localhost");
}

void Config::Save() {
    this->Serialize();
    SaveJsonFile(this->storage, file_path);
}

void Config::Load() {
    if (!ReadJsonFile(this->storage, file_path)) {
        this->values.display_mode_index = 0;
        this->values.window_mode = WindowMode::FULLSCREEN;
        this->values.limit_fps = true;
        this->values.player_name = "juergen";
        this->values.assets_dir = "../../assets";
        this->values.force_localhost = false;
        this->Serialize();
        SaveJsonFile(this->storage, file_path);
    }

    this->Deserialize();
}

void Config::ApplyWindowSettings() {
    auto mode = this->GetDisplayMode();
    auto win = GetGraphicsManager().win;

#if !defined(DEVELOPMENT) || !DEVELOPMENT
    switch (this->window_mode) {
        case Window_Mode::FULLSCREEN:
            SDL_SetWindowBordered(win, SDL_TRUE);
            SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN);
            SDL_SetWindowSize(win, mode.w, mode.h);
            break;

        case Window_Mode::BORDERLESS:
            SDL_SetWindowFullscreen(win, 0);
            SDL_SetWindowBordered(win, SDL_FALSE);
            SDL_SetWindowSize(win, mode.w, mode.h);
            break;

        case Window_Mode::WINDOWED:
            SDL_SetWindowFullscreen(win, 0);
            SDL_SetWindowBordered(win, SDL_TRUE);
            SDL_SetWindowSize(win, mode.w, mode.h);
            break;
    }
#endif

    if (SDL_SetWindowDisplayMode(win, &mode) == -1) {
        FAIL("Failed to set window mode");
    }
}

SDL_DisplayMode Config::GetDisplayMode() const {
    SDL_DisplayMode mode;
    SDL_GetDisplayMode(Config::DEFAULT_DISPLAY_INDEX, this->values.display_mode_index, &mode);
    return mode;
}

Config &GetConfig() {
    static Config res;
    return res;
}
