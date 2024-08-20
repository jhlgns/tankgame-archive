#include "client/client.hpp"
#include "client/console.hpp"
#include "client/config/config.hpp"
#include "common/command_manager.hpp"

void RegisterClientCommands() {
    auto &command_manager = GetCommandManager();

    command_manager.RegisterCommand(
        "exit",
        [](const Array<String> &args) {
            GetClient().Quit();
        });

    command_manager.RegisterCommand(
        "cls",
        [](const Array<String> &args) {
            GetConsole().SetVisible(false);
        });

    command_manager.RegisterCommand(
        "connect",
        [](const Array<String> &args) {
            auto print_help = []() {
                LogError("connect command", "usage: connect <ip>");
            };

            String ip;

            if (!GetArg(args, 0, ip)) {
                print_help();
            } else {
                GetClient().SetNextState(client_states::MakeConnecting(ip));
            }
        });

    command_manager.RegisterCommand(
        "config",
        [](const Array<String> &args) {
            auto print_help = []() {
                LogError("config command", "usage: config get <key> | config set <key> <value>");
            };

            String mode;
            String key;
            if (!GetArg(args, 0, mode) ||
                !GetArg(args, 1, key)) {
                print_help();
                return;
            }

            auto &config = GetConfig();

            if (mode == "set") {
                String value;
                if (!GetArg(args, 2, value)) {
                    print_help();
                    return;
                }

                config.storage[key] = value;
                config.Deserialize();
                config.Save();
                LogInfo("config command", "Set {}: {}"_format(key, value));
            } else if (mode == "get") {
                auto it = config.storage.find(key);
                if (it != config.storage.end()) {
                    LogInfo("config command", "{}: {}"_format(key, it->dump()));
                } else {
                    LogInfo("config command", "Could not find value by key '{}'"_format(key));
                }
            } else {
                print_help();
            }
        });

}
