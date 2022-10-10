#include "Window.hpp"

#include <stdexcept>
#include <utility>

Window::Window(WindowSettings window_settings)
: m_window_settings(std::move(window_settings))
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, m_window_settings.resizable);

    auto* const display = m_window_settings.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    m_handle = glfwCreateWindow(m_window_settings.width(),
                                m_window_settings.height(),
                                m_window_settings.title.c_str(),
                                display, nullptr);

    if (m_handle == nullptr)
    {
        throw std::runtime_error("Failed to create window.");
    }
}

Window::~Window()
{
    if (m_handle != nullptr)
    {
        glfwDestroyWindow(m_handle);
    }
    glfwTerminate();
}

std::vector<const char*> Window::get_instance_extensions()
{
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    return std::vector<const char*>(extensions, extensions + extensionCount);
}

vk::Extent2D Window::get_framebuffer_extent() const
{
    int w, h;
    glfwGetFramebufferSize(m_handle, &w, &h);
    return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}

vk::Extent2D Window::get_window_extent() const
{
    int w, h;
    glfwGetWindowSize(m_handle, &w, &h);
    return { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
}
