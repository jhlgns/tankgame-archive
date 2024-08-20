//
// TODO
//   - cache the uniform locations?
//

#include "client/graphics/shader.hpp"

#include "common/log.hpp"
#include "client/graphics/texture.hpp"

#include <fstream>

static Optional<String> ReadFileAsString(StringView path) {
    std::ifstream ifs{path.data()};

    if (!ifs.is_open()) {
        LogError("shader", "Could not read file {}. File does not exist."_format(path));
        return std::nullopt;
    }

    return String{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

static bool LinkProgram(GLuint &out, GLuint *shaders, size_t n, bool del_shaders) {
    GLuint prg = glCreateProgram();

    for (i32 i = 0; i < n; ++i) {
        glAttachShader(prg, shaders[i]);
    }

    if (del_shaders) {
        for (size_t i = 0; i < n; ++i) {
            glDeleteShader(shaders[i]);
        }
    }

    glLinkProgram(prg);
    GLint success;
    glGetProgramiv(prg, GL_LINK_STATUS, &success);

    if (!success) {
        char buf[512];
        glGetProgramInfoLog(prg, sizeof(buf), nullptr, buf);
        LogError("shader", "Failed to link program: {}"_format(buf));
        return 0;
    }

    out = prg;
    return 1;
}

bool CompileShader(GLuint& out, GLuint type, StringView source) {
    auto shd = glCreateShader(type);
    auto source_data = source.data();
    glShaderSource(shd, 1, &source_data, nullptr);
    glCompileShader(shd);
    GLint success;
    glGetShaderiv(shd, GL_COMPILE_STATUS, &success);

    if (!success) {
        char buf[512];
        glGetShaderInfoLog(shd, sizeof(buf), nullptr, buf);
        LogError("shader", "Failed to compile shader: {}"_format(buf));
        return false;
    }

    out = shd;
    return true;
}

static Optional<GLuint> CompileProgram(StringView vertex_shader_source, StringView fragment_shader_source) {
    GLuint shaders[2];

    if (!CompileShader(shaders[0], GL_VERTEX_SHADER, vertex_shader_source) ||
        !CompileShader(shaders[1], GL_FRAGMENT_SHADER, fragment_shader_source)) {
        return std::nullopt;
    }

    GLuint program;

    if (!LinkProgram(program, shaders, 2, true)) {
        return std::nullopt;
    }

    // TODO delete shaders now

    return program;
}

Shader::~Shader() {
    glDeleteProgram(this->program_id);
}

bool Shader::Init(StringView vertex, StringView fragment, bool from_file) {
    Optional<String> vertex_code;
    Optional<String> fragment_code;

    if (from_file) {
        vertex_code = ReadFileAsString(vertex);
        fragment_code = ReadFileAsString(fragment);
        if (!vertex_code.has_value() || !fragment_code.has_value()) {
            LogError("shader", "Could not load shader source code");
            return false;
        }
    } else {
        vertex_code = vertex;
        fragment_code = fragment;
    }

    auto prg = CompileProgram(vertex_code.value(), fragment_code.value());
    if (!prg.has_value()) {
        LogError("shader", "Program could not be compiled/linked");
        return false;
    }

    this->program_id = prg.value();
    return true;
}

void Shader::SetUniform(StringView name, bool value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniform1i(location, static_cast<GLint>(value));
}

void Shader::SetUniform(StringView name, i32 value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniform1i(location, value);
}

void Shader::SetUniform(StringView name, f32 value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniform1f(location, value);
}

void Shader::SetUniform(StringView name, Mat4 value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
 }

void Shader::SetUniform(StringView name, Vec2 value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniform2fv(location, 1, glm::value_ptr(value));
}

void Shader::SetUniform(StringView name, Vec3 value) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void Shader::SetUniform(const String &name, const Texture &texture) {
    GLuint last_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    defer { glUseProgram(last_program); };

    glUseProgram(this->program_id);
    auto location = glGetUniformLocation(this->program_id, name.data());
    if (location == -1) {
        return;
    }

    auto it = this->textures.find(location);
    if (it == this->textures.end()) {
        GLint max_units = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
        if (this->textures.size() + 1 > max_units) {
            assert(false);
            LogError("shader", "Can't set texture uniform: out of texture image units");
            return;
        }

        this->textures.emplace(location, &texture);
    } else {
        it->second = &texture;
    }
}

void Shader::BindTextures() {
    size_t index = 1;
    for (const auto &[location, texture] : this->textures) {
        glUniform1i(location, index);
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, texture->id);
        ++index;
    }

    glActiveTexture(GL_TEXTURE0);
}
