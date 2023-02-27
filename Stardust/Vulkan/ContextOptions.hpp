#pragma once

#include <set>
#include <GLFW/glfw3.h>

namespace sdvk
{
    struct ContextOptions
    {
        std::set<const char*> instance_extensions;
        std::set<const char*> instance_layers;
        bool validation { false };
        bool debug { false };

        bool with_surface = { false };
        GLFWwindow* window { nullptr };

        std::set<const char*> device_extensions;
        bool raytracing = false;
    };
}