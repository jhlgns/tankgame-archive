#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"

class OptionsState : public ClientState {
    void Begin() override {
    }

    void End() override {
    }

    void Input(const SDL_Event &e) override {
#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE: {
                        GetClient().Quit();
                    } break;
                }
            }
        }
#endif
    }

    void Tick(f32 dt) override {
    }

    void Render() override {
        auto &client = GetClient();
        GetGraphicsManager().DrawBackground(client.assets.textures.main_menu_background);
        client.gui.options_menu.Show();
    }

    StringView GetDisplayName() const override { return "OptionsState"; }
};

UniquePtr<ClientState> client_states::MakeOptions() {
    return std::make_unique<OptionsState>();
}
