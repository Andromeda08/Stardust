#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "Utility/Macro.hpp"
#include "Struct/WindowSettings.hpp"

class Window {
public:
    NON_COPIABLE(Window)

    explicit Window(WindowSettings  window_settings);

    ~Window();

    vk::Extent2D get_framebuffer_extent() const;

    vk::Extent2D get_window_extent() const;

    GLFWwindow* handle() const { return m_handle; }

    /**
     * @return list of vulkan instance extensions required by glfw.
     */
    static std::vector<const char*> get_instance_extensions();

private:
    GLFWwindow*    m_handle;
    WindowSettings m_window_settings;
};