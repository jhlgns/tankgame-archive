#pragma once

#include "common/common.hpp"
#include <charconv>

struct Command {
    using Callback = std::function<void(const Array<String> &/*args*/)>;
    using AutocompleteCallback = std::function<Array<String>(size_t /*arg_index*/, StringView /*text*/)>;

    Callback callback;
    AutocompleteCallback autocomplete_callback;
};

template<typename T>
bool GetArg(const Array<String> &args, size_t i, T &out) {
    if (i >= args.size()) {
        return false;
    }

    const auto &arg = args[i];

    if constexpr (std::is_same_v<T, String>) {
        out = arg;
        return true;
    } else {
        auto [p, ec] = std::from_chars(arg.begin(), arg.end(), out);
        return ec == std::errc{};
    }
}

struct CommandManager {
    using CommandCallback = std::function<void(const Array<String> &)>;

    void RegisterCommand(const String &name, Command::Callback callback, Command::AutocompleteCallback = nullptr);
    void Dispatch(const String &name, const Array<String> &args);
    Array<String> Autocomplete(const String &name, StringView text, size_t arg_index) const;

    HashMap<String, Command> commands;
    Optional<StringView> running_command;
};

CommandManager &GetCommandManager();
