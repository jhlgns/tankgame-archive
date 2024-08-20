#include "client/console.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "client/graphics/vertex_array.hpp"
#include "common/string_util.hpp"
#include "fasel/token.hpp"
#include "fasel/highlighting.hpp"
#include "common/frame_timer.hpp"
#include "common/command_manager.hpp"
#include <cctype>

static f32 EaseInOutExpo(float fraction) {
    if (fraction < 0.5) {
        return (std::pow(2.0f, 16.0f * fraction) - 1.0f) / 510.0f;
    } else {
        return 1.0f - 0.5f * std::pow(2.0f, -16.0f * (fraction - 0.5f));
    }
}

static void DrawRect(Vec2 position, Vec2 size, Color color) {
    VertexArray va;
    va.Add(Vec2{}, Vec2{1.0f, 1.0f}, position, size, color);
    GetGraphicsManager().Draw(va, GetClient().assets.textures.white_pixel, Camera{});
}

void Animation::run(Callback callback, Curve curve, f32 duration, f32 start, f32 end) {
    this->is_running = true;
    this->start_tick = GetFrameTimer().total_time;
    this->callback = ToRvalue(callback);
    this->curve = curve;
    this->duration = duration;
    this->start = start;
    this->end = end;
}

Animation::Result Animation::Update() {
    if (!this->is_running) {
        return Result::NOT_RUNNING;
    }

    auto elapsed_ticks = GetFrameTimer().total_time - this->start_tick;
    auto is_done = elapsed_ticks >= this->duration;
    auto value = std::lerp(this->start, this->end, this->curve(elapsed_ticks / this->duration));
    this->callback(value, is_done);

    if (is_done) {
        this->is_running = false;
        return Result::DONE;
    }

    return Result::NOT_DONE;
}

void Console::PrintLine(StringView text, Color color) {
    Array<FancyTextRange> ranges;
    ranges.emplace_back(FancyTextRange{
        .start = 0,
        .end = static_cast<i32>(text.size()),
        .color = color,
        });
    this->lines.emplace_back(text, ToRvalue(ranges));
}

void Console::PrintLine(StringView text, Array<FancyTextRange> &&ranges) {
    this->lines.emplace_back(text, ToRvalue(ranges));
}

bool Console::HandleInput(const SDL_Event &event) {
    if (!this->visible) {
        if (event.type == SDL_TEXTINPUT) {
            if ((event.text.text == StringView{"/"} ||
                event.text.text == StringView{":"})) {
                // NOTE(janh) When the control key is pressed, no SDL_TEXTINPUT is dispatched, so opening in fullscreen like
                // this does not work. For now, if you want the console to be fullscreen, enter "f" as the command.
                auto keyboard_state = SDL_GetKeyboardState(NULL);
                auto open_fullscreen = (event.key.keysym.mod & KMOD_CTRL) != 0;
                this->SetVisible(true, open_fullscreen);

                return true;
            } else if (event.text.text == StringView{"("}) {
                this->SetVisible(true, true);
                return true;
            }
        }

        return false;
    }

    switch (event.type) {
        case SDL_KEYDOWN: {
            auto shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
            auto ctrl =  (event.key.keysym.mod & KMOD_CTRL)  != 0;

            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: {
                    if (this->fullscreen) {
                        this->SetVisible(true, false);
                    } else {
                        this->SetVisible(false);
                    }
                } break;

                case SDLK_RETURN: {
                    if (!this->current_input.empty()) {
                        this->ExecuteCommand(this->current_input);

                        // Insert into history but avoid duplication
                        if (this->history.empty() || this->history.back() != this->current_input) {
                            this->history.emplace_back(ToRvalue(this->current_input));
                        }

                        this->history_offset = 0;
                        this->current_input.clear();
                        this->SetCursorIndex(0, false, false);
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
                    // NOTE(janh): if the current input is not empty, we should only show matching history entries
                    if (!this->history.empty()) {
                        auto history_entry = *(this->history.rbegin() + this->history_offset);

                        this->SetCursorIndex(0, false, false);
                        this->SetCursorIndex(this->current_input.size(), true, false);
                        this->InsertText(history_entry);

                        this->history_offset = std::clamp(this->history_offset + 1, 0, static_cast<i32>(this->history.size()) - 1);
                    }
                } break;

                case SDLK_DOWN: {
                    if (!this->history.empty()) {
                        auto history_entry = *(this->history.rbegin() + this->history_offset);

                        this->SetCursorIndex(0, false, false);
                        this->SetCursorIndex(this->current_input.size(), true, false);
                        this->InsertText(history_entry);

                        this->history_offset = std::clamp(this->history_offset - 1, 0, static_cast<i32>(this->history.size()) - 1);
                    }
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
                    if (!this->current_autocomplete.empty()) {
                        auto autocompleted = this->current_autocomplete.front();
                        this->SetCursorIndex(0, false, false);
                        this->SetCursorIndex(this->current_input.size(), true, false);
                        this->DeleteSelection();
                        this->InsertText(autocompleted);
                    }
                } break;

                case SDLK_SPACE: {
                    if (ctrl && !this->current_autocomplete.empty()) {
                        auto autocompleted = this->current_autocomplete.front();
                        this->SetCursorIndex(0, false, false);
                        this->SetCursorIndex(this->current_input.size(), true, false);
                        this->DeleteSelection();
                        this->InsertText(autocompleted);
                    }
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

void Console::Tick(f32 dt) {
    auto now = chrono::high_resolution_clock::now();
    if (now > this->last_cursor_toggle + 500ms) {
        this->SetCursorVisible(!this->cursor_visible);
    }
}

void Console::Render() {
    if (!this->visible) {
        return;
    }

    auto &font = GetClient().assets.fonts.default_font;

    this->window_size_animation.Update();
    this->cursor_move_animation.Update();

    auto line_height = 35.0f;
    auto screen_size = GetGraphicsManager().GetWindowSize();
    Vec2 background_size{screen_size.x, this->current_height};
    Vec2 bottom_left_corner{0.0f, screen_size.y - background_size.y};

    { // Draw the background
        Color background_color{0, 0, 0, 170};
        DrawRect(bottom_left_corner, background_size, background_color);
    }

    Vec2 input_size{screen_size.x, line_height};
    auto text_start = bottom_left_corner + Vec2{15.0f, line_height / 2.0f - font.line_spacing / 2.0f};
    auto line_pos = text_start;

    { // Draw input line
        Color input_color{30, 30, 30, 200};
        DrawRect(bottom_left_corner, input_size, input_color);
        //Vec2 input_padding{4.0f, 4.0f};
        //draw_rect(bottom_left_corner + input_padding, input_size - 2.0f * input_padding, Color{45, 45, 45, 255});

        auto highlighted = generate_syntax_highlighting(this->current_input);

        if (!this->current_autocomplete.empty()) {
            auto &autocomplete = this->current_autocomplete.front();
            auto remaining = autocomplete.substr(this->current_input.size());
            highlighted.emplace_back(FancyTextRange{
                .start = static_cast<i32>(this->current_input.size()),
                .end = static_cast<i32>(this->current_input.size() + remaining.size()),
                .color = Color{127, 127, 127, 127}
                });
            DrawString(font, line_pos, this->current_input + remaining, highlighted);
        } else {
            DrawString(font, line_pos, this->current_input, highlighted);
        }

        line_pos.y += line_height;
    }

    // Draw buffered lines
    for (auto it = this->lines.rbegin(); it != this->lines.rend(); ++it) {
        DrawString(font, line_pos, it->text, it->ranges);
        line_pos.y += line_height;
    }

    // Cursor
    auto line_size = font.MeasureString(SafeSubstring(this->current_input, 0, this->cursor_index));

    if (this->cursor_visible) {
        Vec2 cursor_size{15.0f, line_height};
        Color cursor_color{150, 50, 50, 100};
        this->cursor_position = Vec2{text_start.x + line_size.x, bottom_left_corner.y}; // TODO(janh) This disables animation. Remove this when the cursor move animation is back
        //draw_rect(this->cursor_position, cursor_size, cursor_color);
        DrawRect(this->cursor_position, Vec2{1.0f, cursor_size.y}, Color{100, 10, 10, 255});
    }

    // Selection
    if (this->selection_start != -1) {
        auto [start, end] = this->GetSelectionRange();
        auto size_until_start = font.MeasureString(SafeSubstring(this->current_input, 0, start));
        auto size_until_end = font.MeasureString(SafeSubstring(this->current_input, 0, end));
        Vec2 selection_position{text_start.x + size_until_start.x, bottom_left_corner.y};
        Vec2 selection_size{size_until_end.x - size_until_start.x, input_size.y};
        //Vec2 selection_position{text_start.x + size_until_start.x, text_start.y};
        //Vec2 selection_size{size_until_end.x - size_until_start.x, size_until_start.y};
        DrawRect(selection_position, selection_size, Color{50, 50, 255, 127});
    }
}

void Console::SetVisible(bool visible, bool open_fullscreen) {
    if (this->visible == visible && this->fullscreen == open_fullscreen) {
        return;
    }

    this->selection_start = -1;
    this->fullscreen = open_fullscreen;

    f32 height_animation_start = this->current_height;;
    f32 height_animation_end;
    auto screen_size = GetGraphicsManager().GetWindowSize();

    if (visible) {
        this->visible = true,
        height_animation_end = screen_size.y / (fullscreen ? 1.0f : 3.0f);
    } else {
        this->is_closing = true;
        height_animation_end = 0.0f;
    }

    this->window_size_animation.run(
        [this](f32 value, bool is_done) {
            this->current_height = value;

            if (is_done) {
                if (this->is_closing) {
                    this->is_closing = false;
                    this->visible = false;
                } else if (this->fullscreen) {

                }
            }
        },
        EaseInOutExpo, 15.0f, height_animation_start, height_animation_end);
}

void Console::SetCursorVisible(bool visible) {
    this->last_cursor_toggle = chrono::high_resolution_clock::now();
    if (visible != this->cursor_visible) {
        this->cursor_visible = visible;
    }
}

void Console::ExecuteCommand(StringView command) {
    if (command == "f") {
        this->SetVisible(true, !this->fullscreen);
    } else {
        auto tokens = Tokenize(command);

        /*log_info("Console", "================TOKENIZE TEST================");
        log_info("TOKENIZE TEST STRING", command);
        for (const auto &token : tokens) {
            String token_type = "???";

            if (token.type != TOK_NONE && static_cast<i32>(token.type) < 255) {
                token_type = "char {}"_format(static_cast<char>(token.type));
            } else {
                switch (token.type) {
                    case TOK_NONE:             token_type = "NONE"; break;
                    case TOK_MULTILINECOMMENT: token_type = "MULTILINECOMMENT"; break;
                    case TOK_ONELINECOMMENT:   token_type = "ONELINECOMMENT"; break;
                    case TOK_IDENT:            token_type = "IDENT"; break;
                    case TOK_LITERAL:          token_type = "LITERAL"; break;
                    case TOK_WHITESPACE:       token_type = "WHITESPACE"; break;
                    case TOK_PLUSPLUS:         token_type = "PLUSPLUS"; break;
                    case TOK_PLUSEQUALS:       token_type = "PLUSEQUALS"; break;
                    case TOK_MINUSMINUS:       token_type = "MINUSMINUS"; break;
                    case TOK_MINUSEQUALS:      token_type = "MINUSEQUALS"; break;
                    case TOK_RIGHTARROW:       token_type = "RIGHTARROW"; break;
                    case TOK_SLASHEQUALS:      token_type = "SLASHEQUALS"; break;
                    case TOK_STAREQUALS:       token_type = "STAREQUALS"; break;
                    case TOK_EQUALEQUALS:      token_type = "EQUALEQUALS"; break;
                    case TOK_IF:               token_type = "IF"; break;
                    case TOK_THEN:             token_type = "THEN"; break;
                    case TOK_ELSE:             token_type = "ELSE"; break;
                    case TOK_WHILE:            token_type = "WHILE"; break;
                    case TOK_FOR:              token_type = "FOR"; break;
                    case TOK_DEFER:            token_type = "DEFER"; break;
                    case TOK_STRUCT:           token_type = "STRUCT"; break;
                    case TOK_ENUM:             token_type = "ENUM"; break;
                    case TOK_END_OF_FILE:      token_type = "END_OF_FILE"; break;
                }
            }

            auto token_text = safe_substring_range(command, token.start, token.one_past_end);
            log_info(token_type, "'{}'"_format(token_text));
        }*/

        if (tokens.empty()) {
            return;
        }

        auto command_token = tokens.front();
        String command_name{SafeSubstringRange(command, command_token.start, command_token.one_past_end)};

        Array<String> args;
        for (size_t i = 1; i < tokens.size(); ++i) {
            auto arg_token = tokens[i];

            if (arg_token.type == TOK_WHITESPACE ||
                arg_token.type == TOK_MULTILINECOMMENT ||
                arg_token.type == TOK_ONELINECOMMENT ||
                arg_token.type == TOK_NONE) {
                continue;
            }

            auto token_string =
                (arg_token.type == TOK_LITERAL && arg_token.literal_type == LITERAL_STRING)
                ? SafeSubstringRange(command, arg_token.start + 1, arg_token.one_past_end - 1)
                : SafeSubstringRange(command, arg_token.start, arg_token.one_past_end);
            args.emplace_back(ToRvalue(token_string));
        }

        GetCommandManager().Dispatch(command_name, args);
    }
}

void Console::MoveCursor(i32 offset, bool extend_selection, bool wordwise) {
    this->SetCursorIndex(this->cursor_index + offset, extend_selection, wordwise);
}

void Console::SetCursorIndex(i32 index, bool extend_selection, bool wordwise) {
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
        enum class CharGroup { ZERO, PRIMARY, SPACE, SPECIAL, };

        auto char_group = [](char c) {
            if (c == '\0') {
                return CharGroup::ZERO;
            } else if (c < 0 || std::isalpha(c) || std::isdigit(c)) {
                return CharGroup::PRIMARY;
            } else if (std::isspace(c)) {
                return CharGroup::SPACE;
            } else {
                return CharGroup::SPECIAL;
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
                    if (current_group == CharGroup::SPACE) {
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
                    if (i == this->cursor_index || old_group == CharGroup::SPACE) {
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
}

void Console::InsertText(StringView text) {
    this->DeleteSelection();
    this->current_input.insert(this->current_input.begin() + this->cursor_index, text.begin(), text.end());
    this->MoveCursor(text.size(), false, false);
    this->UpdateAutocomplete();
}

void Console::DeleteSelection() {
    this->SetCursorVisible(true);

    if (this->selection_start >= 0 && this->selection_start != this->cursor_index) {
        auto [start, end] = this->GetSelectionRange();
        this->current_input.erase(this->current_input.begin() + start, this->current_input.begin() + end);
        this->selection_start = -1;
        this->cursor_index = start;
        this->UpdateAutocomplete();
    }
}

void Console::UpdateAutocomplete() {
    this->current_autocomplete.clear();
    if (!this->current_input.empty()) {
        this->current_autocomplete = GetCommandManager().Autocomplete(this->current_input, this->current_input, 0);
    }
}

std::pair<i32, i32> Console::GetSelectionRange() const {
    i32 start = this->cursor_index;
    i32 end = this->selection_start;
    if (start > end) {
        std::swap(start, end);
    }

    return std::make_pair(start, end);
}

Console &GetConsole() {
    static Console res;
    return res;
}

