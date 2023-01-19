#include "Application.hpp"

#include <Vulkan/ContextBuilder.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>

namespace sd
{
    Application::Application(const ApplicationOptions& options)
    : m_options(options)
    {
        m_window = std::make_unique<Window>(m_options.window_options);

        m_context = sdvk::ContextBuilder()
                .add_instance_extensions(m_window->get_vk_extensions())
                .add_instance_extensions({ VK_KHR_SURFACE_EXTENSION_NAME })
                .set_validation(true)
                .set_debug_utils(true)
                .with_surface(m_window->handle())
                .add_device_extensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME })
                .add_raytracing_extensions(true)
                .create_context();

        m_command_buffers = std::make_unique<sdvk::CommandBuffers>(8, *m_context);

        m_swapchain = sdvk::SwapchainBuilder(*m_window, *m_context).with_defaults().create();
    }

    void Application::run()
    {
        auto main_loop = [&](){};

        m_window->while_open(main_loop);
    }
}