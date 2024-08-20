#include "client/client.hpp"
#include "client/graphics/graphics.hpp"
#include "common/log.hpp"

#include <filesystem>

int main(int argc, char **argv) {
    assert(argc > 0);

#ifdef WINDOWS
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    std::filesystem::current_path(std::filesystem::path{argv[0]}.remove_filename());
#endif // WINDOW

    int res = EXIT_SUCCESS;
    auto &client = GetClient();

    if (!client.Init()) {
        LogError("client main", "Failed to initialize");
        res = 1;
    } else {
        client.MainLoop();
    }

    client.Deinit();

#ifdef WINDOWS
    WSACleanup();
#endif // WINDOWS

    LogInfo("client main", "Bye");

    return res;
}
