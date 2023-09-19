#include "GraphContext.hpp"
#include <Application/Application.hpp>
#include <Scene/Scene.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace Nebula::RenderGraph
{
    RenderGraphContext::RenderGraphContext(const sdvk::CommandBuffers& command_buffers,
                                           const sdvk::Context& context,
                                           const sdvk::Swapchain& swapchain)
    : m_command_buffers(command_buffers), m_context(context), m_swapchain(swapchain)
    {
        set_render_resolution(sd::Application::s_extent.vk_ext());
        set_target_resolution(sd::Application::s_extent.vk_ext());
    }
}