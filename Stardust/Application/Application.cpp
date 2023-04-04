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
                .add_device_extensions({
                    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
                    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
                })
                .add_raytracing_extensions(true)
                .create_context();

        m_command_buffers = std::make_unique<sdvk::CommandBuffers>(8, *m_context);

        m_swapchain = sdvk::SwapchainBuilder(*m_window, *m_context)
            .set_preferred_format(vk::Format::eB8G8R8Srgb)
            .with_defaults()
            .create();

        m_gui = std::make_unique<sd::GUI>(*m_context, *m_swapchain, *m_window);

        m_scene = std::make_unique<Scene>(*m_command_buffers, *m_context, *m_swapchain);

        m_node = std::make_unique<sd::rg::OffscreenRenderNode>(*m_context, *m_swapchain, *m_scene);
        m_node->compile();
    }

    void Application::run()
    {
        m_window->while_open([&](){
            m_scene->register_keybinds(m_window->handle());

            auto acquired_frame = m_swapchain->acquire_frame(m_current_frame);

            auto command_buffer = m_command_buffers->begin(m_current_frame);
            m_scene->rasterize(m_current_frame, command_buffer);

            m_node->execute(command_buffer);
            command_buffer.end();

            m_swapchain->submit_and_present(m_current_frame, acquired_frame, command_buffer);
            m_current_frame = (m_current_frame + 1) % m_swapchain->image_count();

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Render();
            ImDrawData* draw_data = ImGui::GetDrawData();

            m_context->device().waitIdle();
        });
    }
}