#pragma once

#include "common/common.hpp"

struct Texture;

struct Shader {
    Shader() = default;
    Shader(const Shader &) = delete;
    Shader& operator=(const Shader &) = delete;
    Shader(Shader &&) = delete;
    Shader& operator=(Shader &&) = delete;
    ~Shader();

    bool Init(StringView vertex, StringView fragment, bool from_file);
    void SetUniform(StringView name, bool value);
    void SetUniform(StringView name, i32 value);
    void SetUniform(StringView name, f32 value);
    void SetUniform(StringView name, Mat4 value);
    void SetUniform(StringView name, Vec2 value);
    void SetUniform(StringView name, Vec3 value);
    void SetUniform(const String &name, const Texture &texture);
    void BindTextures();

    u32 program_id;

    HashMap<u32, const Texture *> textures;
};
