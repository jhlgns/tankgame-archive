#pragma once

#include "common/common.hpp"
#include "client/graphics/glad.h"

struct Texture {
    Texture() = default;
    Texture(const Texture &) = delete;
    Texture &operator =(const Texture &) = delete;
    Texture(Texture &&) = default;
    Texture &operator =(Texture &&) = default;
    ~Texture();
    bool LoadFromFile(StringView path);
    void LoadFromImage(
        const unsigned char *image,
        Vec2i dim,
        GLuint format,
        GLuint internal_format = GL_RGBA,
        GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR,
        GLenum mag_filter = GL_LINEAR);
    void Create(
        Vec2i dim,
        GLuint format,
        GLuint internal_format = GL_RGBA,
        GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR,
        GLenum mag_filter = GL_LINEAR);
    void Copy(
        const unsigned char *subimage,
        Vec2i dest,
        Vec2i size,
        GLuint format);
    void Reset();

    inline void Reset(GLuint id, Vec2 dim) {
        this->Reset();
        this->id = id;
        this->dim = dim;
    }

    GLuint id = 0;
    Vec2i dim;
};

struct Vertex {
    Vec2 position;
    Vec2 tex_coords;
    Color color;
};

/*struct VertexNormal {
    Vec2 position;
    Vec2 tex_coords;
    Color color;
    Vec3 normal;
    Vec3 tangent;
    Vec3 bitangent;
};*/
