#include "client/graphics/texture.hpp"
#include "common/log.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::~Texture() {
    this->Reset();
}

bool Texture::LoadFromFile(StringView path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    UniquePtr<unsigned char> image{stbi_load(path.data(), &width, &height, &channels, STBI_default)};
    stbi_set_flip_vertically_on_load(0);

    if (image == nullptr) {
        LogWarning("texture", "Failed to load image file {}"_format(path));
        return false;
    }

    GLenum format = GL_RGB;

    switch (channels) {
        case STBI_rgb:       format = GL_RGB; break;
        case STBI_rgb_alpha: format = GL_RGBA; break;
        default:             assert(false);
    }

    this->LoadFromImage(image.get(), Vec2i{width, height}, format);

    return true;
}

void Texture::LoadFromImage(
    const unsigned char *image,
    Vec2i dim,
    GLuint format,
    GLuint internal_format,
    GLenum min_filter,
    GLenum mag_filter) {
    this->Create(dim, format, internal_format, min_filter, mag_filter);

    glBindTexture(GL_TEXTURE_2D, this->id); {
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // XXX
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, dim.x, dim.y, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    } glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Create(
    Vec2i dim,
    GLuint format,
    GLuint internal_format,
    GLenum min_filter,
    GLenum mag_filter) {
    this->Reset();
    this->dim = dim;

    glGenTextures(1, &this->id);
    glBindTexture(GL_TEXTURE_2D, this->id); {
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // XXX
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, dim.x, dim.y, 0, format, GL_UNSIGNED_BYTE, nullptr);
        //glTexStorage2D(GL_TEXTURE_2D, 0, internal_format, dim.x, dim.y);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
        //glGenerateMipmap(GL_TEXTURE_2D);
    } glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Copy(
    const unsigned char *subimage,
    Vec2i dest,
    Vec2i size,
    GLuint format) {
    glBindTexture(GL_TEXTURE_2D, this->id); {
        glTexSubImage2D(GL_TEXTURE_2D, 0, dest.x, dest.y, size.x, size.y, format, GL_UNSIGNED_BYTE, subimage);
    } glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Reset() {
    if (this->id != 0) {
        glDeleteTextures(1, &this->id);
        this->id = 0;
        this->dim = Vec2{};
    }
}
