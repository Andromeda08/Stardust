#pragma once

#include <initializer_list>
#include <memory>
#include <set>
#include <vector>
#include <GLFW/glfw3.h>
#include "Context.hpp"
#include "ContextOptions.hpp"

namespace sdvk
{
    struct ContextBuilder
    {
        ContextBuilder() = default;

        #pragma region Instance
        ContextBuilder& add_instance_extensions(std::vector<const char*> const& extensions);

        ContextBuilder& add_instance_extensions(std::initializer_list<const char*> const& extensions);

        ContextBuilder& set_validation(bool validation);

        ContextBuilder& set_debug_utils(bool debug);
        #pragma endregion

        #pragma region Presentation
        ContextBuilder& with_surface(GLFWwindow* target);
        #pragma endregion

        #pragma region Device
        ContextBuilder& add_device_extensions(std::initializer_list<const char*> const& extensions);

        ContextBuilder& add_raytracing_extensions(bool flag = false);
        #pragma endregion

        std::unique_ptr<Context> create_context() const;

    private:
        ContextOptions _options;
    };
}