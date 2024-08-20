#include "client/graphics/graphics_manager.hpp"
#include "client/graphics/vertex_array.hpp"
#include "client/graphics/texture.hpp"
#include "client/client.hpp"
#include "client/config/config.hpp"

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);
}

bool GraphicsManager::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        LogError("graphics manager", "Failed to initialize SDL");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    auto &config = GetConfig();
    config.Load();
    auto display_mode = config.GetDisplayMode();
    this->win = SDL_CreateWindow("Tankgame", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        display_mode.w, display_mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!this->win) {
        LogError("graphics manager", "Failed to create window: {}"_format(SDL_GetError()));
        return false;
    }

    config.ApplyWindowSettings();

    this->glctx = SDL_GL_CreateContext(this->win);

    if (!gladLoadGL()) {
        LogError("graphics manager", "Glad could not load GL!\n");
        return false;
    }

    LogInfo("graphics manager", "OpenGL version: {}.{}"_format(GLVersion.major, GLVersion.minor));

    glGenFramebuffers(1, &this->fbo);
    glGenFramebuffers(1, &this->multisampled_fbo);
    glGenRenderbuffers(1, &this->rbo);
    glEnable(GL_MULTISAMPLE);

    unsigned char white_pixel_image[] = {255, 255, 255, 255};
    GetClient().assets.textures.white_pixel.LoadFromImage(white_pixel_image, Vec2i{1, 1}, GL_RGBA);

    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);

    return true;
}

void GraphicsManager::Shutdown() {
    SDL_DestroyWindow(this->win);
    SDL_GL_DeleteContext(this->glctx);
}

void GraphicsManager::BeginFrame() {
    //this->cam.zoom = std::clamp(this->cam.zoom, 0.5f, 2.0f);
    auto screen_dim = this->GetWindowSize();
    glViewport(0, 0, screen_dim.x, screen_dim.y);

    glClearColor(
        this->clear_color.r / 255.0f,
        this->clear_color.g / 255.0f,
        this->clear_color.b / 255.0f,
        1.0f
        );

    glClear(GL_COLOR_BUFFER_BIT);
}

void GraphicsManager::EndFrame() {
    for (const auto &debug_rect : this->debug_rects) {
        VertexArray va;
        va.Add(
            Vec2{},
            Vec2{1.0f, 1.0f},
            Vec2{debug_rect.rect.x, debug_rect.rect.y},
            Vec2{debug_rect.rect.width, debug_rect.rect.height},
            debug_rect.color);
        this->Draw(va, GetClient().assets.textures.white_pixel, Camera{});
    }

    this->debug_rects.clear();

    SDL_GL_SwapWindow(this->win);
}


void GraphicsManager::DrawLines(VertexArray& vertex_array, const Texture& texture,  const Camera& cam) {
    vertex_array.Commit();

    auto shader = &GetClient().assets.shaders.texture;
    auto camera_transform = cam.GetTransform();;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vertex_array.vao); {
        glUseProgram(shader->program_id); {
            shader->SetUniform("Camera", camera_transform);
            shader->SetUniform("Texture", texture);
            shader->BindTextures();

            glDrawArrays(GL_LINES, 0, vertex_array.num_vertices);
        } glUseProgram(0);
    } glBindVertexArray(0);
}

#if 0
void Graphics_Manager::draw(Vertex_Array& vertex_array, const Texture& texture, const Camera& cam, const Shader* shader) {
    vertex_array.commit();

    if (!shader) {
        shader = &get_client().assets.shaders.texture;
    }

    auto camera_transform = cam.get_transform();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vertex_array.get_vao()); {
        shader->bind(); {
            shader->set_mat4("camera", camera_transform);
            glBindTexture(GL_TEXTURE_2D, texture.get_id()); {
                glDrawArrays(GL_TRIANGLES, 0, vertex_array.size());
            } glBindTexture(GL_TEXTURE_2D, 0);
        } shader->unbind();
    } glBindVertexArray(0);
}
#endif

void GraphicsManager::Draw(VertexArray &vertex_array, const Texture &texture, const Camera &cam, Shader *shader, InstanceArray *instances) {
    if (instances != nullptr) {
        instances->SendData(vertex_array);
    }

    vertex_array.Commit();
    auto camera_transform = cam.GetTransform();

    if (shader == nullptr) {
        shader = &GetClient().assets.shaders.texture;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vertex_array.vao); {
        glUseProgram(shader->program_id); {
            // NOTE: Every shader that uses the default texture that is passed to this function
            // as an argument should name the uniform sampler "Texture".
            shader->SetUniform("Texture", texture);
            shader->SetUniform("Camera", camera_transform);
            shader->BindTextures();

            if (instances != nullptr) {
                glDrawArraysInstanced(GL_TRIANGLES, 0, vertex_array.num_vertices, instances->models.size());
            } else {
                glDrawArrays(GL_TRIANGLES, 0, vertex_array.num_vertices);
            }
        } glUseProgram(0);
    } glBindVertexArray(0);
}

void GraphicsManager::DrawBackground(const Texture &texture) {
    auto screen_dim = this->GetWindowSize();
    VertexArray vertex_array;
    vertex_array.Add(
        Vec2{},
        Vec2{1.0f, 1.0f},
        Vec2{},
        screen_dim,
        Color{255, 255, 255, 255});

    this->Draw(vertex_array, texture, Camera{}, nullptr);
}

Vec2i GraphicsManager::GetMousePosition() const {
    Vec2i mouse_position;
    SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
    return mouse_position;
}

Vec2 GraphicsManager::GetWindowSize() const {
    Vec2i window_size;
    SDL_GetWindowSize(GetGraphicsManager().win, &window_size.x, &window_size.y);
    return window_size;
}

void GraphicsManager::PushDebugRect(const FloatRect &bounds, Color color) {
    DebugRect rect{.rect = bounds, .color = color};
    this->debug_rects.push_back(rect);
}

GraphicsManager &GetGraphicsManager() {
    static GraphicsManager res;
    return res;
}

#if 0
bool Graphics_Manager::generate_assets(Texture &planet_texture) {
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);
    Vec2 texture_size{1000.0f, 1000.0f};

    GLuint tex;
    glBindFramebuffer(GL_FRAMEBUFFER, this->multisampled_fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, texture_size.x, texture_size.y, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    GLuint planet_texture_id;
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    glGenTextures(1, &planet_texture_id);
    glBindTexture(GL_TEXTURE_2D, planet_texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_size.x, texture_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    planet_texture.reset(planet_texture_id, Vec2{texture_size.x, texture_size.y});
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, planet_texture.get_id(), 0);


    glBindFramebuffer(GL_FRAMEBUFFER, this->multisampled_fbo);
    glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Array<Draw_Request_Circle> draw_reqs;
    glViewport(0, 0, texture_size.x, texture_size.y);

    draw_reqs.emplace_back(Draw_Request_Circle{
            .position = texture_size / 2.0f,
            .radius = texture_size.x / 2.0f,
            .num_vertices = 2880,
            .color = {255.0f, 255.0f, 255.0f, 255.0f}
        });

    this->draw(Vertex_Array{draw_reqs.data(), draw_reqs.size()}, this->diffuse_shader, Vec2(texture_size.x, texture_size.y));

    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampled_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->fbo);
    glBlitFramebuffer(0, 0, texture_size.x, texture_size.y, 0, 0, texture_size.x, texture_size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, planet_texture.id, 0);
    //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, texture_size.x, texture_size.y, GL_TRUE);
    //glBlitFramebuffer(0, 0, texture_size.x, texture_size.y, 0, 0, texture_size.x, texture_size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    //glBindRenderbuffer(GL_RENDERBUFFER, this->rbo);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1000, 1000);
    //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->rbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glViewport(0, 0, screen_dim.x, screen_dim.y);

    return 1;
}
#endif
