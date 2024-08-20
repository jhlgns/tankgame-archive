#include "client/gui.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <fstream>

#include "client/client.hpp"
#include "client/config/config.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "client/client_game_state.hpp"
#include "server/session.hpp"
#include "common/log.hpp"

#include "json.hpp"
#include "stb_image.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL3_IMPLEMENTATION
#include "nuklear_sdl_gl3.h"

#define MAX_VERTEX_MEMORY  512 * 1024
#define MAX_ELEMENT_MEMORY 512 * 1024

static bool sound_button_label(nk_context *ctx, const char *label) {
    if (nk_button_label(ctx, label)) {
        GetClient().PlaySample(GetClient().assets.sounds.button_click);
        return true;
    }

    return false;
}

static void PushWindowTransparent(nk_context *ctx) {
    nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0, 0, 0, 0));
    nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0, 0, 0, 0)));
    nk_style_push_color(ctx, &ctx->style.window.border_color, nk_rgba(0, 0, 0, 0));
    nk_style_push_color(ctx, &ctx->style.window.group_border_color, nk_rgba(0, 0, 0, 0));
}

static void PopWindowTransparent(nk_context *ctx) {
    nk_style_pop_style_item(ctx);
    nk_style_pop_color(ctx);
    nk_style_pop_color(ctx);
    nk_style_pop_color(ctx);
}

static u32 GuiImageLoad(const char *filename) {
    int x, y, n;
    b32 tex;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
    if (!data) {
        LogError("gui", "Failed to load image: {}"_format(filename));
        return 0;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return tex;
}






void GuiState::Init() {
    auto &client = GetClient();
    auto assets_dir = GetConfig().values.assets_dir;

    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    nk_font* font;
    {
        const void *image;
        int w, h;
        nk_font_atlas *atlas;
        nk_sdl_font_stash_begin(&atlas);
        auto font_path = assets_dir + "/font/MYRIADPRO-REGULAR.OTF";
        font = nk_font_atlas_add_from_file(atlas, font_path.c_str(), 18.0f, NULL);
        image = nk_font_atlas_bake(atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
        nk_sdl_device_upload_atlas(image, w, h);
        nk_sdl_font_stash_end();
    }

    auto win = GetGraphicsManager().win;
    client.gui.ctx = nk_sdl_init(win, font);
    auto ctx = client.gui.ctx;
    SDL_GL_GetDrawableSize(win, &ctx->w, &ctx->h);

    glEnable(GL_TEXTURE_2D);
    std::ifstream i{assets_dir + "/gui/gui.json"};
    using json = nlohmann::json;
    json j;
    i >> j;

    auto gui_height = j.at("meta").at("size").at("h").get<u16>();
    auto gui_width  = j.at("meta").at("size").at("w").get<u16>();
    auto gui_skin = GuiImageLoad((assets_dir + "/gui/gui.png").c_str());

    auto button = j.at("frames").at("button_default.png").at("frame");
    auto gui_button = nk_subimage_id(gui_skin, gui_width, gui_height, nk_rect(button.at("x"), button.at("y"), button.at("w"), button.at("h")));
    button = j.at("frames").at("button_hovered.png").at("frame");
    auto gui_button_hover = nk_subimage_id(gui_skin, gui_width, gui_height, nk_rect(button.at("x"), button.at("y"), button.at("w"), button.at("h")));
    button = j.at("frames").at("button_clicked.png").at("frame");
    auto gui_button_active = nk_subimage_id(gui_skin, gui_width, gui_height, nk_rect(button.at("x"), button.at("y"), button.at("w"), button.at("h")));

    auto background = j.at("frames").at("window.png").at("frame");
    auto gui_background = nk_subimage_id(gui_skin, gui_width, gui_height, nk_rect(background.at("x"), background.at("y"), background.at("w"), background.at("h")));
    auto header = j.at("frames").at("title.png").at("frame");
    auto gui_header = nk_subimage_id(gui_skin, gui_width, gui_height, nk_rect(header.at("x"), header.at("y"), header.at("w"), header.at("h")));

    ctx->style.button.normal = nk_style_item_image(gui_button);
    ctx->style.button.hover = nk_style_item_image(gui_button_hover);
    ctx->style.button.active = nk_style_item_image(gui_button_active);

    //ctx->style.window.fixed_background = nk_style_item_image(gui_background);
    ctx->style.window.border = 5.0f;
    //ctx->style.window.padding = nk_vec2(20, 20);
    //ctx->style.window.header.padding = nk_vec2(10, 0);
    //ctx->style.window.header.spacing = nk_vec2(100, 100);
    //ctx->style.window.header.normal = nk_style_item_image(gui_header);
    //ctx->style.window.header.active = nk_style_item_image(gui_header);
    //ctx->style.window.header.hover = nk_style_item_image(gui_header);
    //ctx->style.window.fixed_background = nk_null_rect;

    ctx->style.window.fixed_background = nk_style_item_color(nk_rgba(6, 12, 18, 220));
    ctx->style.window.border_color = nk_rgba(11, 22, 33, 200);
    ctx->style.window.header.normal = nk_style_item_color(nk_rgba(6, 12, 18, 220));
    ctx->style.window.header.active = nk_style_item_color(nk_rgba(6, 12, 18, 220));
    ctx->style.window.header.hover = nk_style_item_color(nk_rgba(6, 12, 18, 220));

    auto &options_menu = client.gui.options_menu;
    auto &config = GetConfig();
    options_menu.display_mode_index = config.values.display_mode_index;
    options_menu.window_mode = static_cast<int>(config.values.window_mode);
    options_menu.limit_fps = static_cast<int>(config.values.limit_fps);
}

void GuiState::BeginInput() {
    nk_input_begin(GetClient().gui.ctx);
}

void GuiState::HandleEvent(SDL_Event *e) {
    nk_sdl_handle_event(e);
}

void GuiState::EndInput() {
    nk_input_end(GetClient().gui.ctx);
}

void GuiState::Render() {
    nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
}

void GuiState::ShowError() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    if (!client.error_message.has_value()) {
        return;
    }

    if (nk_begin(ctx, "Error", nk_rect(700, 500, 300, 200), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE)) {
        nk_layout_row_static(ctx, 50, 300, 1);
        nk_label(ctx, client.error_message->c_str(), NK_TEXT_ALIGN_CENTERED);

        if (sound_button_label(ctx, "Close")) {
            client.error_message.reset();
        }
    } nk_end(ctx);
}

void MainMenu::Show() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    PushWindowTransparent(ctx);

    if (nk_begin(ctx, "MainMenu", nk_rect(0.1f * ctx->w, 0.8f * ctx->h, 0.8f * ctx->w, 0.08f * ctx->h),
        NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(ctx, 0.08f * ctx->h, 3);

        /*if (sound_button_label(ctx, "Singleplayer")) {
            printf("Singleplayer\n");
        }*/

        if (sound_button_label(ctx, "Multiplayer")) {
            GetClient().SetNextState(client_states::MakeConnecting());
        }

        if (sound_button_label(ctx, "Options")) {
            client.SetNextState(client_states::MakeOptions());
        }

        /*if (sound_button_label(ctx, "Credits")) {
            printf("Credits\n");
        }*/

        if (sound_button_label(ctx, "Exit")) {
            client.Quit();
        }
    } nk_end(ctx);

    PopWindowTransparent(ctx);

    client.gui.ShowError();
}

void SessionBrowserMenu::Show() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;
    this->password_buffers.resize(this->session_infos->size());

    if (nk_begin(ctx, "Sessionbrowser", nk_rect(0.2f * ctx->w, 0.1f * ctx->h, 0.6f * ctx->w, 0.8f * ctx->h), NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {
        float ratio[] = { 0.3f, 0.1f, 0.2f, 0.3f, 0.1f };

        nk_layout_row(ctx, NK_DYNAMIC, 30, 5, ratio);
        nk_label(ctx, "Name", NK_TEXT_LEFT);
        nk_label(ctx, "Players", NK_TEXT_LEFT);
        nk_label(ctx, "Status", NK_TEXT_LEFT);
        nk_label(ctx, "Password", NK_TEXT_LEFT);

        for (size_t i = 0; i < this->session_infos->size(); ++i) {
            int dummy;

            auto& info = this->session_infos->at(i);
            char nplayers[16];
            snprintf(nplayers, sizeof(nplayers), "%d/%d", (int)info.nplayers_connected, (int)info.nplayers);
            nk_layout_row(ctx, NK_DYNAMIC, 30, 5, ratio);
            nk_label(ctx, info.name.c_str(), NK_TEXT_LEFT);
            nk_label(ctx, nplayers, NK_TEXT_LEFT);
            nk_label(ctx, "", NK_TEXT_LEFT);

            if (info.haspw) {
                nk_edit_string(ctx, NK_EDIT_SIMPLE, this->password_buffers[i].data(), &dummy, this->password_buffers[i].size() - 1, nk_filter_default);
            } else {
                nk_label(ctx, "", NK_TEXT_LEFT);
            }

            if (sound_button_label(ctx, "Join")) {
                JoinSessionRequest request;
                request.session_id = info.id;
                request.player_name = GetConfig().values.player_name;
                request.password = this->password_buffers[i].data();
                client.Send(request);
            }
        }

        PushWindowTransparent(ctx);
        nk_layout_space_begin(ctx, NK_STATIC, 60, INT_MAX);
        auto bounds = nk_window_get_bounds(ctx);
        auto screen_space = nk_rect(bounds.x, bounds.y + bounds.h * 0.9f, bounds.w, bounds.h * 0.1f);
        auto local_space = nk_layout_space_rect_to_local(ctx, screen_space);
        nk_layout_space_push(ctx, local_space);

        if (nk_group_begin(ctx, "Nav", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, bounds.h * 0.08f, 3);

            if (sound_button_label(ctx, "Back")) {
                client.SetNextState(client_states::MakeMenu());
            }

            if (sound_button_label(ctx, "Reload")) {
                this->session_infos->clear();

                GetSessionInfoRequest request;
                client.Send(request);
            }
            if (sound_button_label(ctx, "Create Session")) {
                client.SetNextState(client_states::MakeCreateSession());
            }
            nk_group_end(ctx);
        }

        nk_layout_space_end(ctx);
        PopWindowTransparent(ctx);

    } nk_end(ctx);
}

void SessionLobbyMenu::Show() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    if (nk_begin(ctx, "Lobby", nk_rect(0.2f * ctx->w, 0.1f * ctx->h, 0.6f * ctx->w, 0.8f * ctx->h), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {
        PushWindowTransparent(ctx);
        float group_ratio[] = { 0.6f, 0.4f };
        nk_layout_row(ctx, NK_DYNAMIC, 300, 2, group_ratio);

        if (nk_group_begin(ctx, "Playerlist", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            float ratio[] = { 0.5f, 0.3f, 0.2f};
            nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ratio);
            nk_label(ctx, "Player", NK_TEXT_LEFT);
            nk_label(ctx, "Status", NK_TEXT_LEFT);
            nk_label(ctx, "Ping", NK_TEXT_LEFT);
            if (player_info != nullptr) {
                for (auto& player : *player_info) {
                    nk_layout_row(ctx, NK_DYNAMIC, 30, 3, ratio);
                    nk_label(ctx, player.name.c_str(), NK_TEXT_LEFT);
                    if(player.ready) nk_label(ctx, "Ready", NK_TEXT_LEFT);
                    else nk_label(ctx, "Not ready", NK_TEXT_LEFT);
                    nk_label(ctx, "?", NK_TEXT_LEFT);
                }
            }
            nk_group_end(ctx);
        }

        if (nk_group_begin(ctx, "GameOptions", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, 30, 3);
            nk_label(ctx, "Options", NK_TEXT_LEFT);
            nk_group_end(ctx);
        }

        nk_layout_space_begin(ctx, NK_STATIC, 60, INT_MAX);
        auto bounds = nk_window_get_bounds(ctx);
        auto screen_space = nk_rect(bounds.x, bounds.y + bounds.h * 0.9f, bounds.w, bounds.h * 0.1f);
        auto local_space = nk_layout_space_rect_to_local(ctx, screen_space);
        nk_layout_space_push(ctx, local_space);

        if (nk_group_begin(ctx, "Nav", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, bounds.h * 0.08f, 3);

            if (sound_button_label(ctx, "Leave")) {
                LeaveSessionMessage message;
                client.Send(message);
            }

            if (sound_button_label(ctx, "Ready")) {
                ReadyMessage message;
                client.Send(message);
            }
            nk_group_end(ctx);
        }
        nk_layout_space_end(ctx);
        PopWindowTransparent(ctx);
    } nk_end(ctx);
}

void CrateSessionMenu::Show() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    if (nk_begin(ctx, "Create Session", nk_rect(0.2f * ctx->w, 0.1f * ctx->h, 0.6f * ctx->w, 0.8f * ctx->h), NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {
        f32 ratio[] = { 0.5f, 0.5f, 0.2f, 0.3f, 0.1f };
        int dummy;

        nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
        nk_label(ctx, "Sessionname", NK_TEXT_LEFT);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, this->name, &dummy, sizeof(this->name) - 1, nk_filter_default);
        nk_label(ctx, "Password", NK_TEXT_LEFT);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, this->password, &dummy, sizeof(this->password) - 1, nk_filter_default);
        nk_label(ctx, "Max. Players", NK_TEXT_LEFT);
        nk_property_int(ctx, "", 1, &this->max_players, 8, 1, 1);
        nk_label(ctx, "Bots", NK_TEXT_LEFT);
        nk_property_int(ctx, "", 0, &this->num_bots, 8, 1, 1);

        PushWindowTransparent(ctx);
        nk_layout_space_begin(ctx, NK_STATIC, 60, INT_MAX);
        auto bounds = nk_window_get_bounds(ctx);
        auto screen_space = nk_rect(bounds.x, bounds.y + bounds.h * 0.9f, bounds.w, bounds.h * 0.1f);
        auto local_space = nk_layout_space_rect_to_local(ctx, screen_space);
        nk_layout_space_push(ctx, local_space);

        if (nk_group_begin(ctx, "Nav", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, bounds.h * 0.08f, 3);

            if (sound_button_label(ctx, "Back")) {
                client.SetNextState(client_states::MakeSessionbrowser());
            }

            if (sound_button_label(ctx, "Create")) {
                CreateSessionRequest request;
                request.name = this->name;
                request.password = this->password;
                request.num_players = this->max_players;
                request.num_bots = this->num_bots;
                request.player_name = GetConfig().values.player_name;
                client.Send(request);
            }
            nk_group_end(ctx);
        }

        nk_layout_space_end(ctx);
        PopWindowTransparent(ctx);

    } nk_end(ctx);
}

void OptionsMenu::Show() {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    Array<UniquePtr<String>> modes;
    Array<const char *> buffers;

    static const char* window_modes[] = { "Fullscreen", "Borderless", "Windowed" };

    SDL_DisplayMode mode;
    auto display_index = 0;
    auto num_modes = SDL_GetNumDisplayModes(display_index);

    for (int mode_index = 0; mode_index < num_modes; mode_index++) {
        SDL_GetDisplayMode(display_index, mode_index, &mode);

        const auto &formatted = modes.emplace_back(std::make_unique<String>("{}x{} {}hz"_format(mode.w, mode.h, mode.refresh_rate)));
        buffers.emplace_back(formatted->c_str());
    }

    if (nk_begin(ctx, "Options", nk_rect(0.2f * ctx->w, 0.1f * ctx->h, 0.6f * ctx->w, 0.8f * ctx->h), NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE)) {
        if (nk_tree_push(ctx, NK_TREE_TAB, "Graphics", NK_MINIMIZED)) {
            auto &options_menu = client.gui.options_menu;
            nk_layout_row_dynamic(ctx, 0.03f * ctx->h, 2);
            nk_label(ctx, "Resolution", NK_TEXT_LEFT);
            assert(num_modes == buffers.size());
            options_menu.display_mode_index = nk_combo(ctx, buffers.data(), num_modes, options_menu.display_mode_index, 25, nk_vec2(200, 200));

            nk_layout_row_dynamic(ctx, 0.03f * ctx->h, 2);
            nk_label(ctx, "Window mode", NK_TEXT_LEFT);
            options_menu.window_mode = nk_combo(ctx, window_modes, 3, options_menu.window_mode, 25, nk_vec2(200, 200));

            nk_layout_row_dynamic(ctx, 0.03f * ctx->h, 2);
            nk_label(ctx, "Limit FPS", NK_TEXT_LEFT);
            nk_checkbox_label(ctx, "", &options_menu.limit_fps);

            nk_tree_pop(ctx);
        }

        if (nk_tree_push(ctx, NK_TREE_TAB, "Audio", NK_MINIMIZED)) {
            nk_tree_pop(ctx);
        }

        if (nk_tree_push(ctx, NK_TREE_TAB, "Other", NK_MINIMIZED)) {
            nk_tree_pop(ctx);
        }

        PushWindowTransparent(ctx);
        nk_layout_space_begin(ctx, NK_STATIC, 60, INT_MAX);
        auto bounds = nk_window_get_bounds(ctx);
        auto screen_space = nk_rect(bounds.x, bounds.y + bounds.h * 0.9f, bounds.w, bounds.h * 0.1f);
        auto local_space = nk_layout_space_rect_to_local(ctx, screen_space);
        nk_layout_space_push(ctx, local_space);

        if (nk_group_begin(ctx, "Nav", NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(ctx, bounds.h * 0.08f, 3);

            if (sound_button_label(ctx, "Back")) {
                client.SetNextState(client_states::MakeMenu());
            }

            if (sound_button_label(ctx, "Apply")) {
                auto &options_menu = client.gui.options_menu;
                auto &config = GetConfig();
                config.values.display_mode_index = options_menu.display_mode_index;
                config.values.window_mode = static_cast<Config::WindowMode>(options_menu.window_mode);
                config.values.limit_fps = static_cast<bool>(options_menu.limit_fps);
                config.ApplyWindowSettings();
                config.Save();
                auto mode = config.GetDisplayMode();
                ctx->h = mode.h;
                ctx->w = mode.w;
                client.SetNextState(client_states::MakeMenu());
            }

            nk_group_end(ctx);
        }
        nk_layout_space_end(ctx);
        PopWindowTransparent(ctx);
    } nk_end(ctx);
}

void PauseMenu::Show(ClientGameState &game_state) {
    auto &client = GetClient();
    auto ctx = client.gui.ctx;

    //push_window_transparent(ctx);

    if (nk_begin(ctx, "PauseMenu", nk_rect(0.4f * ctx->w, 0.4f * ctx->h, 0.2f * ctx->w, 0.2f * ctx->h), NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(ctx, 0.03f * ctx->w, 1);
        if (sound_button_label(ctx, "Continue")) {
            game_state.is_pause_menu_open = false;
        }

        nk_layout_row_dynamic(ctx, 0.03f * ctx->w, 1);
        if (sound_button_label(ctx, "Options?")) {
        }

        nk_layout_row_dynamic(ctx, 0.03f * ctx->w, 1);
        if (sound_button_label(ctx, "Disconnect")) {
            GetClient().SetNextState(client_states::MakeMenu());
        }
    } nk_end(ctx);

    //pop_window_transparent(ctx);
}
