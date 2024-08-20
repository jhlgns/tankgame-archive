#include "client/client_state.hpp"

#include "client/client.hpp"
#include "client/graphics/graphics_manager.hpp"
#include "common/log.hpp"

class MenuState : public ClientState {
    void Begin() override {
    }

    void End() override {
    }

    void Input(const SDL_Event &e) override {
#if defined(DEVELOPMENT) && DEVELOPMENT
        switch (e.type) {
            case SDL_KEYDOWN: {
                switch (e.key.keysym.sym) {
                    case SDLK_RETURN:
                    case SDLK_SPACE: {
                        GetClient().SetNextState(client_states::MakeConnecting());
                    } break;

                    case SDLK_ESCAPE: {
                        //get_client().quit();
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
        client.gui.main_menu.Show();
    }

    StringView GetDisplayName() const override { return "MenuState"; }
};

UniquePtr<ClientState> client_states::MakeMenu() {
    return std::make_unique<MenuState>();
}
