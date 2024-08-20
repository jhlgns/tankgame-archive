#include "client/graphics/camera.hpp"

#include "client/graphics/graphics_manager.hpp"

void Camera::Zoom(i32 amount) {
    this->zoom_level += amount;
    this->zoom_level = std::clamp(this->zoom_level, -5, 6);
}

f32 Camera::GetZoom() const {
    return 1.0f + this->zoom_level * 0.125f;
}

void Camera::Update() {
    if (this->disable_scroll) {
        return;
    }

    auto mouse_pos = GetGraphicsManager().GetMousePosition();
    auto screen_dim = GetGraphicsManager().GetWindowSize() - Vec2{1.0f, 1.0f};;
    Vec2 direction{};

    // TODO cleanup!!!
    if (mouse_pos.x == 0.0f && mouse_pos.y == 0.0f) {
        direction = Vec2{-1.0f, 1.0f};
    } else if (mouse_pos.x == 0.0f && screen_dim.y > mouse_pos.y) {
        direction = Vec2{-1.0f, 0.0f};
    } else if (mouse_pos.x == 0.0f && mouse_pos.y == screen_dim.y) {
        direction = Vec2{-1.0f, -1.0f};
    } else if (screen_dim.x > mouse_pos.x && mouse_pos.y == screen_dim.y) {
        direction = Vec2{0.0f, -1.0f};
    } else if (mouse_pos.x == screen_dim.x && mouse_pos.y == screen_dim.y) {
        direction = Vec2{1.0f, -1.0f};
    } else if (mouse_pos.x == screen_dim.x && mouse_pos.y == 0.0f) {
        direction = Vec2{1.0f, 1.0f};
    } else if (mouse_pos.x == screen_dim.x && screen_dim.y > mouse_pos.y) {
        direction = Vec2{1.0f, 0.0f};
    } else if (screen_dim.x > mouse_pos.x && mouse_pos.y == 0.0f) {
        direction = Vec2{0.0f, 1.0f};
    }

    if (direction.x != 0.0f || direction.y != 0.0f) {
        this->position += glm::normalize(direction) * this->scroll_speed / this->GetZoom();
    }
}

Mat4 Camera::GetTransform(bool only_projection) const {
    auto screen_dim = GetGraphicsManager().GetWindowSize();
    Vec3 center{this->position + screen_dim / 2.0f, 0.0f};

    Mat4 view{1.0f};
    if (!only_projection) {
        view = glm::translate(Mat4{1.0f}, Vec3{-this->position, 0.0f});
        view =
            glm::translate(Mat4{1.0f}, center) *
            glm::scale(view, Vec3{this->GetZoom(), this->GetZoom(), 1.0f}) *
            glm::translate(Mat4{1.0f}, -center);
    }

    auto projection = glm::ortho(0.0f, screen_dim.x, 0.0f, screen_dim.y, 0.0f, 1.0f);
    return projection * view;
}

Vec2 Camera::ScreenToWorld(Vec2 screen) const {
    auto window_size = GetGraphicsManager().GetWindowSize();
    Vec2 normalized;
    normalized.x = (screen.x / window_size.x) * 2.0f - 1.0f;
    normalized.y = (screen.y / window_size.y) * -2.0f + 1.0f;
    return glm::inverse(this->GetTransform()) * Vec4{normalized, 0.0f, 1.0f};
}

Vec2 Camera::WorldToScreen(Vec2 world) const {
    auto window_size = GetGraphicsManager().GetWindowSize();
    auto normalized = this->GetTransform() * Vec4{world, 0.0f, 1.0f};
    Vec2 pixel;
    pixel.x = (normalized.x + 1.0f) / 2.0f * window_size.x;
    pixel.y = (-normalized.y + 1.0f) / 2.0f * window_size.y;
    return pixel;
}

Vec2 Camera::GetMouseWorldPosition() const {
    return this->ScreenToWorld(Vec2{GetGraphicsManager().GetMousePosition()});
}
