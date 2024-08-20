#pragma once

#include "common/common.hpp"

struct FancyTextRange;
struct Font;
union SDL_Event;

struct TextBox {
    void SetFont(Font *font);
    bool HandleInput(const SDL_Event &event);
    void Tick();
    void Render();
    void SetCursorVisible(bool visible);
    void MoveCursor(i32 offset, bool extend_selection, bool wordwise);
    void SetCursorIndex(i32 index, bool extend_selection, bool wordwise);
    void InsertText(StringView text);
    void SetText(StringView text);
    void DeleteSelection();
    std::pair<i32, i32> GetSelectionRange() const;
    void HandleTextChanged();

    Font *font = nullptr;
    Vec2 position{};
    Vec2 size{};
    Vec2 text_padding{5.0f, 5.0f};
    Vec2 full_size{};
    Color background_color{30, 30, 30, 200};
    i32 cursor_index = 0;
    Vec2 cursor_position{};
    Vec2 cursor_size{};
    Vec2 scroll_offset{};
    bool cursor_visible = true;
    chrono::high_resolution_clock::time_point last_cursor_toggle;
    String current_input;
    i32 selection_start = -1;
    bool multiline = true;

    using KeyDownCallback = std::function<void(TextBox &)>;
    using TextStyleCallback = std::function<void(StringView, Array<FancyTextRange>&, String &)>;

    KeyDownCallback on_enter;
    KeyDownCallback on_tab;
    KeyDownCallback on_up;
    KeyDownCallback on_down;
    TextStyleCallback text_styler;
};
