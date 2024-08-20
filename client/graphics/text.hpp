#pragma once

#include "common/common.hpp"
#include "client/graphics/texture.hpp"

#include <ft2build.h>
//#include FT_FREETYPE_H
#include <freetype/freetype.h>

struct FontGlyph {
    Vec2 advance;
    Vec2 bearing;
    Vec2 atlas_size;
    Vec2 atlas_position;
};

struct Font {
    Font() = default;
    Font(const Font &) = delete;
    Font& operator=(const Font &) = delete;
    Font(Font &&) = delete;
    Font& operator=(Font &&) = delete;

    ~Font();
    bool LoadFromFile(StringView filename, u32 pixel_size);
    const FontGlyph &GetGlyph(char c) const;
    Vec2 MeasureString(StringView text, Vec2 *out_cursor = nullptr) const;

    f32 line_spacing = 0.0f;
    Texture atlas;
    FT_Face face;
    Array<FontGlyph> glyphs;
    f32 y_min = 0.0f;
};

struct FancyTextRange {
    i32 start;
    i32 end;
    Color color;
};

void DrawString(const Font &font, Vec2 position, StringView text, Color color);
void DrawString(const Font &font, Vec2 position, StringView text, const Array<FancyTextRange> &style);
