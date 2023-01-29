#include "Window.hpp"

#include <stdexcept>

namespace sd
{
    Window::Window(const WindowOptions& window_options)
    : m_options(window_options)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, false);

        // If fullscreen mode is requested use the primary monitor.
        GLFWmonitor* display = m_options.fullscreen
                ? glfwGetPrimaryMonitor()
                : nullptr;

        m_window = glfwCreateWindow(m_options.width(), m_options.height(),
                                    m_options.title.c_str(), display, nullptr);

        if (!m_window)
        {
            throw std::runtime_error("Failed to create window.");
        }

        glfwSetKeyCallback(m_window, Window::default_key_handler);
    }

    void Window::while_open(const std::function<void()>& fun)
    {
        while (!glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            fun();
        }
    }

    Window::~Window()
    {
        if (m_window)
        {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

    Extent Window::get_extent() const
    {
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        return {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    }

    Extent Window::get_framebuffer_extent() const
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        return {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    }

    std::vector<const char *> Window::get_vk_extensions()
    {
        uint32_t extension_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        return std::vector<const char*>(extensions, extensions + extension_count);
    }

    void Window::default_key_handler(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}