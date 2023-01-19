#pragma once

#include <functional>
#include <vector>
#include <GLFW/glfw3.h>
#include "WindowOptions.hpp"

namespace sd
{
    class Window
    {
    public:
        Window(Window const&) = delete;
        Window& operator=(Window const&) = delete;

        explicit Window(WindowOptions const& window_options);

        ~Window();

        // The passed function gets executed in every iteration of the loop until the window should close.
        void while_open(std::function<void()> const& function);

        GLFWwindow* handle() const { return m_window; }

        Extent get_extent() const;

        Extent get_framebuffer_extent() const;

        static std::vector<const char*> get_vk_extensions();

    private:
        static void default_key_handler(GLFWwindow* window, int key, int scancode, int action, int mods);

    private:
        GLFWwindow*   m_window;
        WindowOptions m_options;
    };
}