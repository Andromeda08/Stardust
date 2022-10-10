#pragma once

#include <string>
#include <glm/glm.hpp>

struct WindowSettings
{
    std::string title {"Application"};
    glm::ivec2 resolution { 1280, 720 };
    bool fullscreen = false;
    bool resizable  = false;

    int width() const { return resolution.x; }

    int height() const { return resolution.y; }
};
