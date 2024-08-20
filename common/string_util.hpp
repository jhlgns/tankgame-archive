#pragma once

#include "common.hpp"

inline StringView SafeSubstringRange(StringView text, size_t start, size_t end = StringView::npos) {
    start = std::min(start, text.size());
    end   = std::min(end,   text.size());

    if (start >= end) {
        return "";
    }

    return text.substr(start, end - start);
}

inline StringView SafeSubstringRangeInclusive(StringView text, size_t start, size_t end = StringView::npos) {
    start = std::min(start,   text.size());
    end   = std::min(end + 1, text.size());

    if (start >= end) {
        return "";
    }

    return text.substr(start, end - start);
}

inline StringView SafeSubstring(StringView text, size_t start, size_t off = StringView::npos) {
    if (off != StringView::npos) {
        auto end = std::min(text.size(), start + off);
        return SafeSubstringRange(text, start, end);
    } else {
        auto end = StringView::npos;
        return SafeSubstringRange(text, start, end);
    }
}

inline void InplaceTrim(String &text) {
    auto left_it = std::find_if(
        text.begin(),
        text.end(),
        [](auto ch) {
            return !std::isspace(ch);
        });

    auto right_it =
        std::find_if(
            text.rbegin(),
            text.rend(),
            [](auto ch) {
                return !std::isspace(ch);
            }).base();

    text.erase(text.begin(), left_it);
    text.erase(right_it, text.end());
}
