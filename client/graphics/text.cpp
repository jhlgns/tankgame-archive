#include "client/graphics/text.hpp"

#include "client/client.hpp"
#include "client/graphics/glad.h"
#include "client/graphics/graphics_manager.hpp"
#include "client/graphics/vertex_array.hpp"
#include <freetype/ftglyph.h>

Font::~Font() {
    //FT_Done_Face(this->face); TODO
}

bool Font::LoadFromFile(StringView filename, u32 pixel_size) {
    auto water_mark = GetClient().frame_allocator.water_mark;
    defer { GetClient().frame_allocator.water_mark = water_mark; };

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        LogError("font", "Failed to initialize FreeType");
        return false;
    }

    defer { FT_Done_FreeType(ft); };

    if (FT_New_Face(ft, filename.data(), 0, &this->face)) {
        return false;
    }

    FT_Set_Pixel_Sizes(this->face, 0, pixel_size);
    this->line_spacing = this->face->size->metrics.height >> 6;

    constexpr u8 first_char = 32;
    constexpr u8 last_char  = 128;

    this->glyphs.resize(128);
    constexpr Vec2i padding{2, 2};
    Vec2i atlas_size{};

    // Calculate the atlas size
    for (u32 c = first_char; c < last_char; ++c) {
        if (FT_Load_Char(this->face, c, FT_LOAD_RENDER)) {
            LogError("font", "Failed to load glyph for '{}'"_format(static_cast<char>(c)));
            continue;
        }

        auto glyph = this->face->glyph;
        auto &bitmap = glyph->bitmap;

        atlas_size.x += bitmap.width + 2 * padding.x;
        if (bitmap.rows + padding.y > atlas_size.y) {
            atlas_size.y = bitmap.rows + padding.y;
        }

        FT_Glyph the_glyph;
        if (FT_Get_Glyph(face->glyph, &the_glyph)) {
            return false;
        }

        FT_BBox bbox;
        FT_Glyph_Get_CBox(the_glyph, ft_glyph_bbox_pixels, &bbox);
        if (bbox.yMin < this->y_min) {
            this->y_min = bbox.yMin;
        }
    }

    // Create the atlas and store the glyphs
    this->atlas.Create(atlas_size, GL_RED, GL_RED, GL_NEAREST, GL_NEAREST);
    i32 x = padding.x;
    for (u32 c = first_char; c < last_char; ++c) {
        if (FT_Load_Char(this->face, c, FT_LOAD_RENDER)) {
            assert(false);
            continue;
        }

        auto glyph = this->face->glyph;
        auto &bitmap = glyph->bitmap;
        Vec2i bitmap_size{bitmap.width, bitmap.rows};

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // XXX
        this->atlas.Copy(bitmap.buffer, Vec2i{x, padding.y}, bitmap_size, GL_RED);

        this->glyphs[c].advance = Vec2{glyph->advance.x >> 6, glyph->advance.y >> 6};
        this->glyphs[c].bearing = Vec2{glyph->bitmap_left, glyph->bitmap_top};
        this->glyphs[c].atlas_position = Vec2{x, padding.y};
        this->glyphs[c].atlas_size = bitmap_size;
        x += bitmap.width + padding.x;
    }

    return true;
}

const FontGlyph &Font::GetGlyph(char c) const {
    return this->glyphs[c >= this->glyphs.size() ? '?' : c];
}

Vec2 Font::MeasureString(StringView text, Vec2 *out_cursor) const {
    Vec2 res{0.0f, this->line_spacing};
    Vec2 cursor{};

    for (auto c : text) {
        switch (c) {
            case '\n':
                cursor.x = 0.0f;
                cursor.y -= this->line_spacing;
                res.y += this->line_spacing;
                break;

            case '\t':
                cursor.x += 4 /* <-- if you don't like this, you are not welcome here */ * this->GetGlyph(' ').advance.x;
                break;

            default:
                cursor.x += this->GetGlyph(c).advance.x;
        }

        res.x = std::max(res.x, cursor.x);
        //res.y = std::min(res.y, cursor.y);
    }

    if (out_cursor) {
        *out_cursor = cursor;
    }

    return res;
}

void DrawString(const Font &font, Vec2 position, StringView text, Color color) {
    FancyTextRange range{
        .start = 0,
        .end = static_cast<i32>(text.size()),
        .color = color,
    };
    DrawString(font, position, text, {range});
}

void DrawString(const Font &font, Vec2 position, StringView text, const Array<FancyTextRange> &ranges) {
    VertexArray vertex_array;

    Vec2 atlas_size{font.atlas.dim};
    position.y -= font.y_min; // ???
    position = glm::floor(position);

    size_t range_index = 0;
    FancyTextRange current_range;
    current_range.end = -1;
    Vec2 screen_offset{};

    for (i32 i = 0; i < text.size(); ++i) {
        for (; i > current_range.end && range_index < ranges.size(); ++range_index) {
            current_range = ranges[range_index];
        }

        switch (text[i]) {
            case '\n':
                screen_offset.x = 0.0f;
                screen_offset.y -= font.line_spacing;
                continue;

            case '\t':
                screen_offset.x += 4 * font.glyphs[' '].advance.x;
                continue;
        }

        auto &glyph = font.GetGlyph(text[i]);
        auto uv = glyph.atlas_position;
        Vec2 glyph_atlas_size = glyph.atlas_size;
        auto char_pos =
            Vec2{position.x + glyph.bearing.x, position.y - (glyph_atlas_size.y - glyph.bearing.y)} +
            screen_offset;

        // NOTE: The quads are flipped since the font atlas is flipped. Should we consider making a Vertex_Array::add_quad that flips the triangles?
        Vertex verts[6];
        verts[0].position   = char_pos + Vec2{0.0f, glyph_atlas_size.y}; // Top    left
        verts[1].position   = char_pos + Vec2{};                         // Bottom left
        verts[2].position   = char_pos + Vec2{glyph_atlas_size.x, 0.0f}; // Bottom right
        verts[3].position   = char_pos + Vec2{0.0f, glyph_atlas_size.y}; // Top    left
        verts[4].position   = char_pos + Vec2{glyph_atlas_size.x, 0.0f}; // Bottom right
        verts[5].position   = char_pos + Vec2{glyph_atlas_size.x, glyph_atlas_size.y}; // Top right
        verts[0].tex_coords = (uv + Vec2{0.0f, 0.0f})                             / atlas_size; // Bottom left
        verts[1].tex_coords = (uv + Vec2{0.0f, glyph_atlas_size.y})               / atlas_size; // Top    left
        verts[2].tex_coords = (uv + Vec2{glyph_atlas_size.x, glyph_atlas_size.y}) / atlas_size; // Top    right
        verts[3].tex_coords = (uv + Vec2{0.0f, 0.0f})                             / atlas_size; // Bottom left
        verts[4].tex_coords = (uv + Vec2{glyph_atlas_size.x, glyph_atlas_size.y}) / atlas_size; // Top    right
        verts[5].tex_coords = (uv + Vec2{glyph_atlas_size.x, 0.0f})               / atlas_size; // Bottom right
        verts[0].color = current_range.color;
        verts[1].color = current_range.color;
        verts[2].color = current_range.color;
        verts[3].color = current_range.color;
        verts[4].color = current_range.color;
        verts[5].color = current_range.color;

        vertex_array.AddTriangles(verts);

        screen_offset.x += glyph.advance.x;
    }

    GetGraphicsManager().Draw(vertex_array, font.atlas, Camera{}, &GetClient().assets.shaders.font);
}
