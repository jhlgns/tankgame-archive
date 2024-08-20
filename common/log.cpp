#include "log.hpp"

#include <chrono>

#ifdef CLIENT
#include "client/client.hpp"
#include "client/console.hpp"
#endif

String FmtEscape(StringView text) {
    // TODO
    return String{text};
}

static void DoLog(
    StringView label,
    StringView tag,
    StringView message,
    fmt::text_style tag_style,
    fmt::text_style message_style = {},
    Color color = {}) {
    auto time = chrono::system_clock::to_time_t(chrono::system_clock::now());
    auto time_string = fmt::format(
        "{:%H:%M:%S}.{:#03}",
        *std::localtime(&time),
        (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()) % 1000).count());

#if 0
    fmt::print(fmt::fg(fmt::color::cornflower_blue), "{} "_format(time_string));
    fmt::print(tag_style, "{} [{}] "_format(label, tag));
    fmt::print(message_style, "{}\n"_format(fmt_escape(message)));
#else
    fmt::print("{} "_format(time_string));
    fmt::print("{} [{}] "_format(label, tag));
    fmt::print("{}\n"_format(FmtEscape(message)));
#endif

#ifdef CLIENT
    auto full_text = "{} {} [{}] {}"_format(time_string, label, tag, message);

    Color default_color{150, 150, 250, 255};
    FancyTextRange time_range;
    time_range.start = 0;
    time_range.end = time_string.size();
    time_range.color = default_color;

    FancyTextRange label_range;
    label_range.start = time_range.end + 1;
    label_range.end = time_range.end + 1 + label.size();
    label_range.color = color;

    FancyTextRange tag_range;
    tag_range.start = label_range.end + 2;
    tag_range.end = label_range.end + 3 + tag.size();
    tag_range.color = color;

    FancyTextRange message_range;
    message_range.start = tag_range.end + 1;
    message_range.end = tag_range.end + 1 + message.size();
    message_range.color = Color{255, 255, 255, 255};

    GetConsole().PrintLine(full_text, Array<FancyTextRange>{time_range, label_range, tag_range, message_range});
#endif
}

void InitLog() {
    // TODO open logfile
}

void LogInfo(StringView tag, StringView message) {
    DoLog("Info", tag, message, fmt::fg(fmt::color::green), {}, Color{50, 170, 50, 255});
}

void LogMilestone(StringView tag, StringView message) {
    auto style = fmt::fg(fmt::color::light_sea_green) | fmt::emphasis::underline | fmt::emphasis::bold;
    DoLog("Info", tag, message, style, style, Color{50, 50, 170, 255});
}

void LogWarning(StringView tag, StringView message) {
    DoLog("Warn", tag, message, fmt::fg(fmt::color::yellow), {}, Color{170, 170, 50, 255});
}

void LogError(StringView tag, StringView message) {
    DoLog("Err", tag, message, fmt::fg(fmt::color::red), {}, Color{170, 50, 50, 255});
}

void LogDebug(StringView tag, StringView message) {
#if DEVELOPMENT
    DoLog("Debug", tag, message, fmt::fg(fmt::color::lime), {}, Color{170, 50, 50, 255});
#endif
}
