#pragma once

#include "client/graphics/graphics.hpp"
#include "client/graphics/camera.hpp"
//#include "client/graphics/vertex_array.hpp"

#include <SDL.h>
#include <SDL_mixer.h>

struct Client;
struct Shader;
struct Texture;
struct VertexArray;
struct InstanceArray;
struct SDL_Window;

struct DebugRect {
    FloatRect rect;
    Color color;
};

struct GraphicsManager {
    SDL_Window *win = nullptr;
    SDL_GLContext glctx{};
    GLuint fbo;
    GLuint multisampled_fbo;
    GLuint rbo;
    Color clear_color{0, 0, 0, 255};

    bool Init();
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    //void draw(Vertex_Array &vertex_array, const Texture &texture, const Camera &cam, const Shader *shader = nullptr);
    void Draw(VertexArray &vertex_array, InstanceArray &instances, const Texture &diffuse, const Texture &normal, Vec2 light_position, const Camera &cam);
    void Draw(VertexArray &vertex_array, const Texture &texture, const Camera &cam, Shader *shader = nullptr, InstanceArray *instances = nullptr);
    void DrawLines(VertexArray& vertex_array, const Texture& texture, const Camera& cam);
    void DrawBackground(const Texture &tx);
    Vec2i GetMousePosition() const;
    Vec2 GetWindowSize() const;
    void PushDebugRect(const FloatRect &bounds, Color color);

    Array<DebugRect> debug_rects; // Cleared after every frame
};

GraphicsManager &GetGraphicsManager();
