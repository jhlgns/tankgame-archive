#pragma once

#include "common/common.hpp"
#include "client/graphics/glad.h"

struct Camera {
    Vec2 position{};
    i32 zoom_level = 0;
    f32 scroll_speed = 4.0f;
    bool disable_scroll = false;

    void Zoom(i32 amount);
    f32 GetZoom() const;
    void Update();
    Mat4 GetTransform(bool only_projection = false) const;
    Vec2 ScreenToWorld(Vec2 screen) const;
    Vec2 WorldToScreen(Vec2 world) const;
    Vec2 GetMouseWorldPosition() const;
};
