#include "client/gui/text_box.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "client/graphics/vertex_array.hpp"
#include "client/graphics/text.hpp"
#include "common/string_util.hpp"
#include "fasel/token.hpp"
#include "fasel/highlighting.hpp"
#include "common/frame_timer.hpp"
#include "common/command_manager.hpp"
#include <cctype>

static void DrawRect(Vec2 position, Vec2 size, Color color) {
    VertexArray va;
    va.Add(Vec2{}, Vec2{1.0f, 1.0f}, position, size, color);
    GetGraphicsManager().Draw(va, GetClient().assets.textures.white_pixel, Camera{});
}

void TextBox::SetFont(Font *font) {
    this->font = font;
    this->cursor_size = Vec2{1.0f, this->font->line_spacing};
}

bool TextBox::HandleInput(const SDL_Event &event) {
    switch (event.type) {
        case SDL_KEYDOWN: {
            auto shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
            auto ctrl =  (event.key.keysym.mod & KMOD_CTRL)  != 0;

            switch (event.key.keysym.sym) {
                case SDLK_RETURN: {
                    if (this->multiline) {
                        this->InsertText("\n");
                    } else {
                        if (this->on_enter != nullptr) {
                           this->on_enter(*this);
                        }

                        /*if (!this->current_input.empty()) {
                            this->current_input.clear();
                            this->set_cursor_index(0, false, false);
                        }*/
                    }
                } break;

                case SDLK_BACKSPACE: {
                    if (this->selection_start == -1) {
                        this->MoveCursor(-1, true, ctrl);
                    }

                    this->DeleteSelection();
                } break;

                case SDLK_DELETE: {
                    if (this->cursor_index < this->current_input.size() && !this->current_input.empty()) {
                        // We can delete to the right
                        if (this->selection_start == -1) {
                            this->MoveCursor(1, true, ctrl);
                        }
                    } else if (this->cursor_index > 0 && !this->current_input.empty()) {
                        // Otherwise try to do what SDLK_BACKSPACE does
                        if (this->selection_start == -1) {
                            this->MoveCursor(-1, true, ctrl);
                        }
                    }

                    this->DeleteSelection();
                } break;

                case SDLK_LEFT: {
                    this->MoveCursor(-1, shift, ctrl);
                } break;

                case SDLK_RIGHT: {
                    this->MoveCursor(1, shift, ctrl);
                } break;

                case SDLK_UP: {
                    if (this->on_up) {
                        this->on_up(*this);
                    } else if (this->multiline) {
                        i32 line_beginning_1 = this->cursor_index - 1;
                        for (; line_beginning_1 >= 0 && this->current_input[line_beginning_1] != '\n'; --line_beginning_1);

                        i32 line_beginning_2 = line_beginning_1 - 1;
                        for (; line_beginning_2 >= 0 && this->current_input[line_beginning_2] != '\n'; --line_beginning_2);

                        auto line_length_1 = this->cursor_index - 1 - line_beginning_1;
                        auto line_length_2 = line_beginning_1 - 1 - line_beginning_2;

                        this->SetCursorIndex(line_beginning_2 + 1 + (line_length_1 < line_length_2 ? line_length_1 : line_length_2), shift, false);
                    }

                    // NOTE(janh): if the current input is not empty, we should only show matching history entries
                    /*if (!this->history.empty()) {
                        auto history_entry = *(this->history.rbegin() + this->history_offset);

                        this->set_cursor_index(0, false, false);
                        this->set_cursor_index(this->current_input.size(), true, false);
                        this->insert_text(history_entry);

                        this->history_offset = std::clamp(this->history_offset + 1, 0, static_cast<i32>(this->history.size()) - 1);
                    }*/
                } break;

                case SDLK_DOWN: {
                    if (this->on_down) {
                        this->on_down(*this);
                    } else if (this->multiline) {
                        i32 line_beginning_1 = this->cursor_index - 1;
                        for (; line_beginning_1 >= 0 && this->current_input[line_beginning_1] != '\n'; --line_beginning_1);

                        i32 line_beginning_2 = this->cursor_index + 1;
                        for (; line_beginning_2 < this->current_input.size() && this->current_input[line_beginning_2] != '\n'; ++line_beginning_2);

                        auto line_length_1 = this->cursor_index - 1 - line_beginning_1;
                        auto line_length_2 = line_beginning_2 - this->cursor_index + 1;

                        this->SetCursorIndex(line_beginning_2 + 1 + line_length_1, shift, false);
                    }

                    /*if (!this->history.empty()) {
                        auto history_entry = *(this->history.rbegin() + this->history_offset);

                        this->set_cursor_index(0, false, false);
                        this->set_cursor_index(this->current_input.size(), true, false);
                        this->insert_text(history_entry);

                        this->history_offset = std::clamp(this->history_offset - 1, 0, static_cast<i32>(this->history.size()) - 1);
                    }*/
                } break;

                case SDLK_HOME: {
                    this->SetCursorIndex(0, shift, false);
                } break;

                case SDLK_END: {
                    this->SetCursorIndex(this->current_input.size(), shift, false);
                } break;

                case SDLK_c: {
                    if (ctrl) {
                        auto [start, end] = this->GetSelectionRange();
                        auto selection_text = SafeSubstringRange(this->current_input, start, end);
                        SDL_SetClipboardText(selection_text.data());
                    }
                } break;

                case SDLK_v: {
                    if (ctrl) {
                        auto clipboard_text = SDL_GetClipboardText();
                        this->InsertText(clipboard_text);
                        SDL_free(clipboard_text);
                    }
                } break;

                case SDLK_x: {
                    if (ctrl) {
                        auto [start, end] = this->GetSelectionRange();
                        auto selection_text = SafeSubstringRange(this->current_input, start, end);
                        SDL_SetClipboardText(selection_text.data());
                        this->DeleteSelection();
                    }
                } break;

                case SDLK_a: {
                    if (ctrl) {
                        this->SetCursorIndex(0, false, false);
                        this->SetCursorIndex(this->current_input.size(), true, false);
                    }
                } break;

                case SDLK_TAB: {
                    if (this->on_tab) {
                        this->on_tab(*this);
                    }

                    // TODO ontab
                    /*if (!this->current_autocomplete.empty()) {
                        auto autocompleted = this->current_autocomplete.front();
                        this->set_cursor_index(0, false, false);
                        this->set_cursor_index(this->current_input.size(), true, false);
                        this->delete_selection();
                        this->insert_text(autocompleted);
                    }*/
                } break;

                case SDLK_SPACE: {
                    // TODO on_autocopmlete???
                    /*if (ctrl && !this->current_autocomplete.empty()) {
                        auto autocompleted = this->current_autocomplete.front();
                        this->set_cursor_index(0, false, false);
                        this->set_cursor_index(this->current_input.size(), true, false);
                        this->delete_selection();
                        this->insert_text(autocompleted);
                    }*/
                } break;
            }
        } break;

        case SDL_TEXTINPUT: {
            this->InsertText(event.text.text);
        } break;

        case SDL_MOUSEWHEEL: {
            // TODO(janh)
        } break;
    }

    return true;
}

void TextBox::Tick() {
    auto now = chrono::high_resolution_clock::now();
    if (now > this->last_cursor_toggle + 500ms) {
        this->SetCursorVisible(!this->cursor_visible);
    }
}

void TextBox::Render() {
    glEnable(GL_SCISSOR_TEST);
    auto was_scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
    defer {
        if (true || !was_scissor_test_enabled) { // TODO(janh): This breaks the nuklear gui... Why???
            glDisable(GL_SCISSOR_TEST);
        }
    };

    glScissor(this->position.x, this->position.y, this->size.x, this->size.y);

    DrawRect(
        this->position /*-
            //2.0f * this->text_padding -
            Vec2{0.0f, this->full_size.y - this->font->line_spacing} +
            Vec2{0.0f, 2.0f * this->text_padding.y}*/,
        this->size,
        this->background_color);

    /*auto highlighted = generate_syntax_highlighting(this->current_input);

    if (!this->current_autocomplete.empty()) {
        auto &autocomplete = this->current_autocomplete.front();
        auto remaining = autocomplete.substr(this->current_input.size());
        highlighted.emplace_back(Text_Style_Range{
            .start = static_cast<i32>(this->current_input.size()),
            .end = static_cast<i32>(this->current_input.size() + remaining.size()),
            .color = Color{127, 127, 127, 127}
            });
        draw_string(font, line_pos, this->current_input + remaining, highlighted);
    } else {
        draw_string(font, line_pos, this->current_input, highlighted);
    }*/

    auto content_start =
        (this->position - this->scroll_offset) +
        Vec2{this->text_padding.x, this->size.y - this->font->line_spacing - this->text_padding.y};

    if (this->text_styler) {
        String rendered_text;
        Array<FancyTextRange> styles;
        this->text_styler(this->current_input, styles, rendered_text);
        DrawString(*this->font, content_start, rendered_text, styles);
    } else {
        DrawString(*this->font, content_start, this->current_input, Color{255, 0, 0, 255});
    }

    // Cursor
    Vec2 text_end_cursor{};
    this->font->MeasureString(
        SafeSubstring(this->current_input, 0, this->cursor_index),
        &text_end_cursor);

    if (this->cursor_visible) {
        Color cursor_color{150, 255, 50, 255};
        DrawRect(content_start + this->cursor_position, this->cursor_size, cursor_color);
    }

    // Selection
    if (this->selection_start != -1) {
        auto [start, end] = this->GetSelectionRange();

        Vec2 cursor;
        this->font->MeasureString(SafeSubstring(this->current_input, 0, start), &cursor);

        if (start != end) {
            auto line_start = start;
            auto line_end = start;

            while (true) {
                for (;
                    line_end < this->current_input.size() &&
                        line_end < end &&
                        this->current_input[line_end] != '\n';
                    ++line_end) {}

                auto width = 0.0f;

                if (line_start == line_end) {
                    width = 3.0f;
                } else {
                    width = this->font->MeasureString(SafeSubstringRange(this->current_input, line_start, line_end)).x;
                }

                Vec2 selection_size{width, this->font->line_spacing};
                auto selection_position = content_start + cursor;
                DrawRect(selection_position, selection_size, Color{50, 50, 255, 127});

                if (line_end >= this->current_input.size() || line_end >= end) {
                    break;
                }

                cursor.x = 0.0f;
                cursor.y -= this->font->line_spacing;
                line_start = line_end + 1;
                line_end = line_start;
            }
        }
    }
}

void TextBox::SetCursorVisible(bool visible) {
    this->last_cursor_toggle = chrono::high_resolution_clock::now();
    if (visible != this->cursor_visible) {
        this->cursor_visible = visible;
    }
}

void TextBox::MoveCursor(i32 offset, bool extend_selection, bool wordwise) {
    this->SetCursorIndex(this->cursor_index + offset, extend_selection, wordwise);
}

void TextBox::SetCursorIndex(i32 index, bool extend_selection, bool wordwise) {
    this->SetCursorVisible(true);

    if (extend_selection) {
        if (this->selection_start == -1) {
            this->selection_start = this->cursor_index;
        }
    } else {
        if (this->selection_start != -1) {
            this->selection_start = -1;
        }
    }

    if (wordwise) {
        enum class Char_Group { ZERO, PRIMARY, SPACE, SPECIAL, };

        auto char_group = [](char c) {
            if (c == '\0') {
                return Char_Group::ZERO;
            } else if (c < 0 || std::isalpha(c) || std::isdigit(c)) {
                return Char_Group::PRIMARY;
            } else if (std::isspace(c)) {
                return Char_Group::SPACE;
            } else {
                return Char_Group::SPECIAL;
            }
        };

        auto old_group = char_group(this->current_input[this->cursor_index]);

        auto i = this->cursor_index;
        if (index > this->cursor_index) {
            // Forward search for beginning of next group that is not space
            for (; i < this->current_input.size() + 1; ++i) {
                char next_char = i + 1 <= this->current_input.size() ? this->current_input[i + 1] : '\0';
                auto current_group = char_group(next_char);
                if (current_group != old_group) {
                    if (current_group == Char_Group::SPACE) {
                        old_group = current_group;
                    } else {
                        ++i;
                        break;
                    }
                }
            }
        } else {
            // Backward search for beginning of previous group that is not space
            for (; i >= 0; --i) { // Find the beginning of old_group
                auto next_char = i > 0 ? this->current_input[i - 1] : '\0';
                auto current_group = char_group(next_char);
                if (current_group != old_group) {
                    if (i == this->cursor_index || old_group == Char_Group::SPACE) {
                        // If we are already on the beginning of the old group, we skip.
                        // Same goes for 'space' groups.
                        old_group = current_group;
                    } else {
                        break;
                    }
                }
            }
        }

        this->cursor_index = i;
    } else {
        this->cursor_index = index;
    }

    this->cursor_index = std::clamp(this->cursor_index, 0, static_cast<i32>(this->current_input.size()));

    // Update cursor screen position
    this->font->MeasureString(
        SafeSubstring(this->current_input, 0, this->cursor_index),
        &this->cursor_position);

    // Check if the cursor is outside of the visible area. If so, we need to scroll
    FloatRect bounds{this->scroll_offset/* - this->text_padding / 2.0f*/, this->size/* + this->text_padding*/};
    FloatRect cursor_bounds{
            Vec2{0.0f, this->size.y - this->font->line_spacing} +
            this->cursor_position -
            this->text_padding / 2.0f,
        this->cursor_size + this->text_padding};

    auto overhang_left   = bounds.x - cursor_bounds.x;
    auto overhang_right  = (cursor_bounds.x + cursor_bounds.width) - (bounds.x + bounds.width);
    auto overhang_bottom = bounds.y - cursor_bounds.y;
    auto overhang_top    = (cursor_bounds.y + cursor_bounds.height) - (bounds.y + bounds.height);

    if (overhang_left > 0.0f) {
        this->scroll_offset.x -= std::max(overhang_left, this->size.x / 2.0f);
    } else if (overhang_right > 0.0f) {
        this->scroll_offset.x += overhang_right + this->text_padding.x * 2.0f;
    }

    if (overhang_top > 0.0f) {
        this->scroll_offset.y += overhang_top;
    } else if (overhang_bottom > 0.0f) {
        this->scroll_offset.y -= overhang_bottom + this->text_padding.y * 2.0f;
    }

    if (this->scroll_offset.x < 0.0f) this->scroll_offset.x = 0.0f;
    //if (this->scroll_offset.y < 0.0f) this->scroll_offset.y = 0.0f;
}

void TextBox::InsertText(StringView text) {
    this->DeleteSelection();
    this->current_input.insert(this->current_input.begin() + this->cursor_index, text.begin(), text.end());
    this->MoveCursor(text.size(), false, false);
    //this->update_autocomplete();
    this->HandleTextChanged();
}

void TextBox::DeleteSelection() {
    this->SetCursorVisible(true);

    if (this->selection_start >= 0 && this->selection_start != this->cursor_index) {
        auto [start, end] = this->GetSelectionRange();
        this->current_input.erase(this->current_input.begin() + start, this->current_input.begin() + end);
        this->selection_start = -1;
        this->cursor_index = start;
        //this->update_autocomplete();
        this->HandleTextChanged();
    }
}

void TextBox::SetText(StringView text) {
    this->MoveCursor(0, true, false);
    this->MoveCursor(this->current_input.size(), true, false);
    this->InsertText(text);
}

std::pair<i32, i32> TextBox::GetSelectionRange() const {
    assert(this->selection_start != -1);
    i32 start = this->cursor_index;
    i32 end = this->selection_start;
    if (start > end) {
        std::swap(start, end);
    }

    return std::make_pair(start, end);
}

void TextBox::HandleTextChanged() {
    this->full_size = this->font->MeasureString(this->current_input) + 2.0f * this->text_padding;
}
