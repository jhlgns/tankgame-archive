#pragma once

#include "common/common.hpp"
#include "common/game_state.hpp"
#include "common/socket.hpp"
#include "client_state.hpp"
#include "client/gui.hpp"
#include "client/graphics/text.hpp"
#include "client/graphics/shader.hpp"
#include "common/frame_allocator.hpp"

#if defined(LINUX) && LINUX
#	include <SDL2/SDL.h>
#	include <SDL2/SDL_mixer.h>
#else
#	include <SDL.h>
#	include <SDL_mixer.h>
#endif

struct SessionInfo;
struct DisconnectMessage;

struct Assets {
    bool Load();

    struct {
        Texture white_pixel;
        Texture planet_diffuse;
        Texture planet_normal;
        Texture tank_diffuse;
        Texture tank_normal;
        Texture tank_turret_diffuse;
        Texture tank_turret_normal;
        Texture main_menu_background;
        Texture sessionbrowser_background;
        Texture lobby_background;
        Texture ingame_background;
    } textures;

    struct {
        Mix_Music *main_menu;
    } music;

    struct {
        Mix_Chunk *button_click;
        Mix_Chunk *window_open;
        Mix_Chunk *tank_track;
        Mix_Chunk *tank_fire;
        Mix_Chunk *tank_explode;
    } sounds;

    struct  {
        Font default_font;
    } fonts;

    struct {
        Shader texture;
        Shader blend;
        Shader font;
        Shader normal_map;
        Shader ingame_background;
        Shader glowing_projectile;
    } shaders;
};

struct Client {
    Client();
    ~Client();
    Client(const Client &) = delete;
    Client& operator=(const Client &) = delete;
    Client(Client &&) = delete;
    Client& operator=(Client &&) = delete;
    bool Init();
    void Deinit();
    void MainLoop();
    void Input();
    void Tick();
    void Render();
    void Quit();
    void LockStateChange();
    void UnlockStateChange();
    void _SetState(UniquePtr<ClientState> state);
    void SetNextState(UniquePtr<ClientState> state);
    void Disconnect();
    void SendPacket(Packet &pkt);
    void ProtocolError();
    bool CheckSocketError();
    void PlaySample(Mix_Chunk *chunk);
    void HandleDisconnectMessage(DisconnectMessage &&message);

    template<typename T>
    void Send(const T &data) {
        Packet packet;
        data.Serialize(packet);
        this->SendPacket(packet);
    }

    template<typename T>
    void SendGameCommand(const T &command) {
        Packet packet;
        packet.WriteEnum(NetMessageType::GAME_COMMAND);
        packet.WriteEnum(command.type);
        command.Serialize(packet);
        this->SendPacket(packet);
    }

    bool quit_flag = false;
    FrameAllocator frame_allocator;
    bool finish_outbound_packets = false;
    Assets assets;
    UniquePtr<ClientState> state;
    UniquePtr<ClientState> next_state;
    bool defer_state_change = false;
    TcpSocket socket;
    GuiState gui;

    Optional<String> error_message;
};

Client &GetClient();
