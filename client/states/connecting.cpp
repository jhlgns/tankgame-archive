#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"
#include "client/config/config.hpp"

#include <chrono>

class ConnectingState : public ClientState {
public:
    explicit ConnectingState(Optional<StringView> ip) {
        if (ip.has_value()) {
            this->target_ip = ip.value();
            this->try_localhost_on_failure = false;
        } else if (!GetConfig().values.force_localhost) {
            this->target_ip = "85.235.64.169";
        }
    }

    void Begin() override {
        this->ConnectToTargetServer();
    }

    void End() override {
    }

    void Input(const SDL_Event &e) override {
    }

    void Tick(f32 dt) override {
        switch (GetClient().socket.DoConnect()) {
            case SocketResult::DONE:
                LogInfo("connecting", "Connected to {}"_format(this->is_connecting_to_deployment_server ? "deployment server" : "localhost"));
                GetClient().SetNextState(client_states::MakeHandshake());
                break;

            case SocketResult::ERROR:
                if (this->is_connecting_to_deployment_server) {
                    LogInfo("connecting", "Deployment server not available; trying localhost");
                    this->ConnectToLocalhost();
                } else {
                    GetClient().error_message = "Could not connect to server";
                    GetClient().SetNextState(client_states::MakeMenu());
                }

                break;

            default:
#if defined(DEVELOPMENT) && DEVELOPMENT && WINDOWS
                // NOTE(janh) On windows there is a timeout for unresponsive servers... There should be a cleaner way to prevent this
                auto time_since_connecting_started = chrono::high_resolution_clock::now() - this->connecting_started;
                if (time_since_connecting_started > 500ms && this->is_connecting_to_deployment_server) {
                    this->ConnectToLocalhost();
                }
#endif

                break;
        }
    }

    void Render() override {
        GetGraphicsManager().DrawBackground(GetClient().assets.textures.sessionbrowser_background);
    }

    StringView GetDisplayName() const override { return "ConnectingState"; }

    void Connect(StringView address) {
        this->connecting_started = chrono::high_resolution_clock::now();
        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        auto result = inet_pton(AF_INET, address.data(), &server_address.sin_addr);
        assert(result == 1);
        LogInfo("client", "Connecting to {}:{}"_format(inet_ntoa(server_address.sin_addr), server_address.sin_port));
        server_address.sin_port = htons(1303);
        GetClient().socket.Connect(server_address);
    }

    void ConnectToTargetServer() {
        this->is_connecting_to_deployment_server = true;
        this->Connect(target_ip);
    }

    void ConnectToLocalhost() {
        if (this->try_localhost_on_failure) {
            this->is_connecting_to_deployment_server = false;
            this->Connect("127.0.0.1");
        }
    }

    bool is_connecting_to_deployment_server;
    bool try_localhost_on_failure = true;
    chrono::high_resolution_clock::time_point connecting_started;
    String target_ip;
};

UniquePtr<ClientState> client_states::MakeConnecting(Optional<StringView> ip) {
    return std::make_unique<ConnectingState>(ip);
}
