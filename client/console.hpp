#pragma once

#include "common/common.hpp"
#include "client/graphics/text.hpp"

union SDL_Event;

struct ConsoleLine {
    inline ConsoleLine(StringView text, Array<FancyTextRange> &&ranges)
        : text(text)
        , ranges(ToRvalue(ranges)) {
     }

    String text;
    Array<FancyTextRange> ranges;
};

struct Animation {
    enum class Result {
        NOT_RUNNING,
        DONE,
        NOT_DONE
    };

    using Callback = std::function<void(f32 /*progress*/, bool /*is_done*/)>;
    using Curve = float(*)(float);

    void run(Callback callback, Curve curve, f32 duration, f32 start, f32 end);
    Result Update();

    bool is_running = false;
    f32 start_tick = 0.0f;
    Callback callback;
    Curve curve;
    f32 duration = 0.0f;
    f32 start = 0.0f;
    f32 end = 0.0f;
};

struct Console {
    void PrintLine(StringView text, Color color = {255, 255, 255, 255});
    void PrintLine(StringView text, Array<FancyTextRange> &&ranges);
    bool HandleInput(const SDL_Event &event);
    void Tick(f32 dt);
    void Render();
    void SetVisible(bool visible, bool open_fullscreen = false);
    void SetCursorVisible(bool visible);
    void ExecuteCommand(StringView command);
    void MoveCursor(i32 offset, bool extend_selection, bool wordwise);
    void SetCursorIndex(i32 index, bool extend_selection, bool wordwise);
    void InsertText(StringView text);
    void DeleteSelection();
    void UpdateAutocomplete();
    std::pair<i32, i32> GetSelectionRange() const;

    bool visible = false;
    bool fullscreen = false;
    bool is_closing = false;
    f32 current_height = 0.0f; // Animate
    Vec2 cursor_position{}; // Animated | remove
    Array<ConsoleLine> lines;
    Array<String> history;
    i32 history_offset = 0;
    String current_input; // remove
    Array<String> current_autocomplete;
    bool cursor_visible = true;
    i32 selection_start = -1;
    i32 cursor_index = 0;
    chrono::high_resolution_clock::time_point last_cursor_toggle;

    Animation cursor_move_animation;
    Animation window_size_animation;
};

Console &GetConsole();
