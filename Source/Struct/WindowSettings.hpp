#pragma once

#include <string>
#include <glm/glm.hpp>

struct WindowSettings
{
    std::string title {"Application"};
    glm::ivec2 resolution { 1280, 720 };
    bool fullscreen = false;
    bool resizable  = false;

    void setSize(int32_t width, int32_t height)
    {
        resolution = { width, height };
    }

    int width() const { return resolution.x; }

    int height() const { return resolution.y; }
};
