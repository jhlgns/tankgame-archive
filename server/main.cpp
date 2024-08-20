#include "server/server.hpp"
#include "common/log.hpp"

#include <filesystem>

int main(int argc, char **argv) {
#ifdef WINDOWS
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }

    std::filesystem::current_path(std::filesystem::path{argv[0]}.remove_filename());
#endif // WINDOWS

    int res = EXIT_SUCCESS;
    auto &server = GetServer();

    if (!server.Start()) {
        LogError("server main", "Failed to initialize");
        res = 1;
    } else {
        server.MainLoop();
    }

#ifdef WINDOWS
    WSACleanup();
#endif // WINDOWS

    LogInfo("server main", "Bye");

    return res;
}
