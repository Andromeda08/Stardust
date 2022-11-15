#pragma once

#include "WindowSettings.hpp"

struct ApplicationSettings
{
    bool logging = false;

    bool raytracing = true;

    WindowSettings windowSettings = {};
};
