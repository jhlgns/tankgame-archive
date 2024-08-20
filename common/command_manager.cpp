#include "common/command_manager.hpp"

#include "common/log.hpp"

void CommandManager::RegisterCommand(const String &name, Command::Callback callback, Command::AutocompleteCallback autocomplete_callback) {
    auto &command = this->commands[name];
    command.callback = ToRvalue(callback);
    command.autocomplete_callback = ToRvalue(autocomplete_callback);
    /*auto [it, ok] = this->commands.emplace(name, ToRvalue(callback), ToRvalue(autocomplete_callback));
    if (!ok) {
        log_warn("command manager", "Overriding command {}"_format(name));
        it->second = ToRvalue(callback);
    }*/
}

void CommandManager::Dispatch(const String &name, const Array<String> &args) {
    auto it = this->commands.find(name);
    if (it == this->commands.end()) {
        LogWarning("command manager", "Command not found: {}"_format(name));
        return;
    }

    this->running_command = name;
    it->second.callback(args);
}

Array<String> CommandManager::Autocomplete(const String &name, StringView text, size_t arg_index) const {
    Array<String> res;

    if (arg_index == 0) {
        for (const auto &[command_name, command] : this->commands) {
            if (command_name.size() <= text.size()) {
                continue;
            }

            auto prefix = command_name.substr(0, text.size());
            if (prefix == text) {
                res.emplace_back(command_name);
            }
        }
    }

    std::sort(res.begin(), res.end());

    return res;
}

CommandManager &GetCommandManager() {
    static CommandManager res;
    return res;
}

