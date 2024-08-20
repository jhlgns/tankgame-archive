#include "client/client.hpp"

#include "client/client_state.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "common/net_platform.hpp"
#include "common/session_info.hpp"
#include "fasel/test.hpp"
#include "common/file_watcher.hpp"
#include "common/frame_timer.hpp"
#include "common/crc32.hpp"
#include "client/console.hpp"
#include "client/config/config.hpp"

#include "client/gui/text_box.hpp"

//static Text_Box _text_box; // TODO(janh): Remove this

void RegisterClientCommands();

static bool LoadTexture(Texture &texture, StringView filename) {
    auto res = texture.LoadFromFile(filename);
    if (!res) {
        LogWarning("client assets", "Failed to load texture {}"_format(filename));
        return false;
    }

    GetFileWatcher().Watch(filename, [&texture, filename = String{filename}]() {
        auto res = texture.LoadFromFile(filename);
        if (!res) {
            LogWarning("client assets", "Failed to hot reload texture {}"_format(filename));
        }
    });

    return true;
}

static bool LoadShader(Shader &shader, StringView vertex_file, StringView fragment_file) {
    // TODO
    auto res = shader.Init(vertex_file, fragment_file, true);
    if (!res) {
        LogWarning("client assets", "Failed to load shader from vertex shader {} or fragment shader {}"_format(vertex_file, fragment_file));
        return false;
    }

    GetFileWatcher().Watch(vertex_file, [&shader, vertex_file = String{vertex_file}, fragment_file = String{fragment_file}]() {
        auto res = shader.Init(vertex_file, fragment_file, true);
        if (!res) {
            LogWarning("client assets", "Failed to hot reload vertex shader from {}"_format(vertex_file));
        }
    });

    GetFileWatcher().Watch(fragment_file, [&shader, vertex_file = String{vertex_file}, fragment_file = String{fragment_file}]() {
        auto res = shader.Init(vertex_file, fragment_file, true);
        if (!res) {
            LogWarning("client assets", "Failed to hot reload fragment shader from {}"_format(fragment_file));
        }
    });

    return true;
}

static bool LoadAudio(Mix_Chunk *&out, StringView filename) {
    out = Mix_LoadWAV(filename.data());
    if (out == nullptr) {
        LogWarning("client assets", "Failed to load sound {}"_format(filename));
        return false;
    }

    return true;
}

static bool LoadMusic(Mix_Music *&out, StringView filename) {
    out = Mix_LoadMUS(filename.data());
    if (out == nullptr) {
        LogWarning("client assets", "Failed to load music {}"_format(filename));
        return false;
    }

    return true;
}

static bool LoadFont(Font &font, StringView filename, u32 pixel_size) {
    if (!font.LoadFromFile(filename, pixel_size)) {
        LogWarning("client assets", "Failed to load font {}"_format(filename));
        return false;
    }

    return true;
}

bool Assets::Load() {
    LogInfo("client", "Loading assets");

    auto assets_dir = GetConfig().values.assets_dir;

    auto texture_dir = assets_dir + "/texture";
    auto sound_dir = assets_dir + "/sound";
    auto font_dir = assets_dir + "/font";
    auto shader_dir = assets_dir + "/shader";

    auto all_loaded =
        LoadTexture(this->textures.planet_diffuse,            texture_dir + "/planet_diffuse.png") &&
        LoadTexture(this->textures.planet_normal,             texture_dir + "/planet_normal.png") &&
        LoadTexture(this->textures.tank_diffuse,              texture_dir + "/tank_diffuse.png") &&
        LoadTexture(this->textures.tank_turret_diffuse,       texture_dir + "/tank_turret_diffuse.png") &&
        LoadTexture(this->textures.tank_normal,               texture_dir + "/tank_normal.png") &&
        LoadTexture(this->textures.main_menu_background,      texture_dir + "/main_menu.png") &&
        LoadTexture(this->textures.sessionbrowser_background, texture_dir + "/sessionbrowser.png") &&
        LoadTexture(this->textures.lobby_background,          texture_dir + "/lobby.png") &&
        LoadTexture(this->textures.ingame_background,         texture_dir + "/ingame.jpg") &&
        LoadMusic(this->music.main_menu,                      sound_dir   + "/dumpster/music/space.mp3") &&
        LoadAudio(this->sounds.button_click,                  sound_dir   + "/dumpster/gui/Button 8.wav") &&
        LoadAudio(this->sounds.window_open,                   sound_dir   + "/dumpster/gui/Window Open 2.wav") &&
        LoadAudio(this->sounds.tank_track,                    sound_dir   + "/tank-tracks.ogg") &&
        LoadAudio(this->sounds.tank_fire,                     sound_dir   + "/dumpster/tank/fire.flac") &&
        LoadAudio(this->sounds.tank_explode,                  sound_dir   + "/dumpster/tank/explode.flac") &&
        LoadFont(this->fonts.default_font,                    font_dir    + "/consola.ttf", 20) &&
        LoadShader(this->shaders.texture,                     shader_dir  + "/texture.vert", shader_dir + "/texture.frag") &&
        LoadShader(this->shaders.font,                        shader_dir  + "/texture.vert", shader_dir + "/font.frag") &&
        LoadShader(this->shaders.blend,                       shader_dir  + "/texture.vert", shader_dir + "/lighting.frag") &&
        LoadShader(this->shaders.normal_map,                  shader_dir  + "/normal_map.vert", shader_dir + "/normal_map.frag") &&
        LoadShader(this->shaders.ingame_background,           shader_dir  + "/texture.vert", shader_dir + "/ingame.frag");
        true;//LoadShader(this->shaders.glowing_projectile,          shader_dir  + "/texture.vert", shader_dir + "/projectile.frag");

    if (all_loaded) {
        LogInfo("client", "Assets loaded");
    } else {
        LogWarning("client", "Asset loading failed");
    }

    return all_loaded;
}

Client::Client() = default;
Client::~Client() = default;

bool Client::Init() {
    LogMilestone("client", "Initializing");

    this->frame_allocator.Reserve(1 * 1024 * 1024 * 1024); // 1GB

    if (!GetGraphicsManager().Init()) {
        return false;
    }

    this->gui.Init();

    Mix_AllocateChannels(16);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    Mix_Volume(-1, 20);

    Crc32::InitTable();

    this->SetNextState(client_states::MakeMenu());

    if (!this->assets.Load()) {
        return false;
    }

    //this->play_sample(this->assets.sounds.window_open);
    //Mix_PlayMusic(this->assets.music.main_menu, -1);

    LogMilestone("client", "Intialization done.");
    RegisterClientCommands();

    //fasel_test();

    /*_text_box.set_font(&this->assets.fonts.default_font);
    _text_box.position = Vec2{100.0f, 500.5f};
    _text_box.size = Vec2{800.0f, 350.0f};
    _text_box.set_text("jsakdfjksf");*/

    return true;
}

void Client::Deinit() {
    LogInfo("client", "Cleaning up");

    if (this->state) {
        this->state->End();
    }

    this->socket.Close(false);

    GetGraphicsManager().Shutdown();
    Mix_CloseAudio();
    SDL_Quit();
}

void Client::MainLoop() {
    LogInfo("Client", "Starting the main loop");

    auto &timer = GetFrameTimer();
    timer.Start();

    while (true) {
        timer.BeginFrame();

        auto socket_done =
            this->socket.state != SocketState::CONNECTED ||
            !this->finish_outbound_packets ||
            this->socket.send.queue.empty();

        if (this->quit_flag && socket_done) {
            break;
        }

        if (this->socket.DoSend(); this->CheckSocketError()) {
            continue;
        }

        if (this->socket.DoRecv(); this->CheckSocketError()) {
            continue;
        }

        u32 ticks_done = 0;
        while (!timer.FrameDone()) {
            timer.BeginTick();
            this->Input();
            this->Tick();
            this->Render(); // NOTE(janh): We could also pull this out to render as often as we can
            timer.AdvanceTick();

            if (++ticks_done > 100) {
                LogWarning("client", "Cannot keep up the framerate! Did {} ticks in this main loop iteration"_format(ticks_done));
            }
        }
    }

    LogInfo("Client", "Main loop exit");
}

void Client::Input() {
    SDL_Event event;
    this->gui.BeginInput();

    while (SDL_PollEvent(&event)) {
        this->gui.HandleEvent(&event);

        switch (event.type) {
            case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    this->Quit();
                }
            } break;

            case SDL_QUIT: {
                this->Quit();
            } break;
        }

        //_text_box.handle_input(event);
        if (GetConsole().HandleInput(event)) {
            continue;
        }

        if (this->state) {
            this->LockStateChange();
            this->state->Input(event);
            this->UnlockStateChange();
        }
    }

    this->gui.EndInput();
}

void Client::Tick() {
    //_text_box.tick();
    auto dt = GetFrameTimer().dt;
    GetFileWatcher().Update(); // TODO(janh): We don't need to do this every frame!

    Packet incoming_packet;

    GetConsole().Tick(dt);

    while (this->socket.Pop(incoming_packet)) {
        if (this->state) {
            this->LockStateChange();
            this->state->net_message_handlers.HandlePacket(ToRvalue(incoming_packet));
            this->UnlockStateChange();
        }
    }

    if (this->state) {
        this->LockStateChange();
        this->state->Tick(dt);
        this->UnlockStateChange();
    }
}

void Client::Render() {
    GetGraphicsManager().BeginFrame();

    if (this->state) {
        this->LockStateChange();
        this->state->Render();
        this->UnlockStateChange();
    }

    this->gui.Render();
    GetConsole().Render();
    //_text_box.render();

#if defined(DEVELOPMENT) && DEVELOPMENT // TODO(janh) show_fps in config???
    { // Draw fps
        auto timer = GetFrameTimer();
        auto fps = timer.fps_avg;
        Color color{0, 255, 0, 255};

        auto target = 1.0e6f / chrono::duration_cast<chrono::microseconds>(timer.GetTickLength()).count();
        if (std::abs(timer.tick_length_delta.count()) > 0) {
            color = Color{100, 10, 170, 255};
        } else if (std::abs(fps - target) > 0.5f) {
            color = Color{255, 255, 0, 255};
        }

        DrawString(this->assets.fonts.default_font, Vec2{0.0f, 0.0f}, "FPS: {:.3f}/{:.3f}"_format(fps, target), color);
    }
#endif

    GetGraphicsManager().EndFrame();
}

void Client::Quit() {
#if defined(DEVELOPMENT) && DEVELOPMENT && WINDOWS
    ShutdownMessage message;
    this->Send(message);
    this->finish_outbound_packets= true;
#endif

    LogInfo("client", "Setting quit flag");

    if (this->quit_flag) {
        LogWarning("client", "Client already quit");
        return;
    }

    this->quit_flag = true;
}

void Client::LockStateChange() {
    // TODO(janh): This might be changed recursively?...
    assert(!this->defer_state_change);
    this->defer_state_change = true;
}

void Client::UnlockStateChange() {
    // TODO(janh): This might be changed recursively?...
    assert(this->defer_state_change);
    this->defer_state_change = false;

    if (this->next_state != nullptr) {
        this->_SetState(ToRvalue(this->next_state));
    }
}

void Client::SetNextState(UniquePtr<ClientState> state) {
    if (this->defer_state_change) {
        assert(this->next_state == nullptr);
        this->next_state = ToRvalue(state);
    } else {
        this->_SetState(ToRvalue(state));
    }
}

void Client::_SetState(UniquePtr<ClientState> new_state) {
    assert(!this->defer_state_change);
    StringView old_display_name = "<none>";
    StringView new_display_name = "<none>";

    if (this->state) {
        old_display_name = this->state->GetDisplayName();
    }

    if (new_state) {
        new_display_name = new_state->GetDisplayName();
    }

    LogInfo("client", "State change: [{}] -> [{}]"_format(old_display_name, new_display_name));

    if (this->state) {
        this->LockStateChange();
        this->state->End();
        this->UnlockStateChange();
    }

    if (this->state = ToRvalue(new_state); this->state != nullptr) {
        this->LockStateChange();
        this->state->net_message_handlers.Add(&Client::HandleDisconnectMessage, this);
        this->state->Begin();
        this->UnlockStateChange();
    }
}

void Client::Disconnect() {
    assert(false);
    this->socket.Close(false);
}

void Client::ProtocolError() {
    this->Disconnect();
    this->SetNextState(client_states::MakeMenu());
}

void Client::SendPacket(Packet &pkt) {
    pkt.WriteHeader();
    this->socket.Push(pkt);
}

bool Client::CheckSocketError() {
    if (this->socket.state == SocketState::ERROR) {
        this->socket.Close(false);
        LogInfo("Client", "Network error");

        if (!this->error_message.has_value()) {
            this->error_message = "Network error";
        }

        this->SetNextState(client_states::MakeMenu());
        return true;
    }

    return false;
}

void Client::PlaySample(Mix_Chunk *chunk) {
    Mix_PlayChannel(-1, chunk, 0);
}

void Client::HandleDisconnectMessage(DisconnectMessage &&message) {
    this->error_message = "Disconnected from server: {} (message: {})"_format(ToString(message.reason), message.message);
}

Client &GetClient() {
    static Client res;
    return res;
}
