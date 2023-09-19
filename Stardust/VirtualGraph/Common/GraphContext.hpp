#pragma once

#include <memory>
#include <VirtualGraph/Compile/RenderPath.hpp>

namespace sd
{
    class Scene;
}

namespace sdvk
{
    class Context;
    class Swapchain;
}

namespace Nebula
{
    class RenderGraphContext
    {
    public:
        RenderGraphContext(const sdvk::Context& context,
                           const sdvk::Swapchain& swapchain);

        const sdvk::Context& context() const { return m_context; }

        const sdvk::Swapchain& swapchain() const { return m_swapchain; }

        const std::shared_ptr<sd::Scene>& scene() const { return m_selected_scene; }

        const vk::Extent2D& render_resolution() const { return m_render_resolution; }

        const vk::Extent2D& target_resolution() const { return m_target_resolution; }

        void set_scene(const std::shared_ptr<sd::Scene>& scene)
        {
            m_selected_scene = std::shared_ptr<sd::Scene>(scene);
        }

        void set_render_path(const std::shared_ptr<RenderGraph::RenderPath>& render_path)
        {
            m_render_path = render_path;
        }

        void set_render_resolution(const vk::Extent2D& render_resolution)
        {
            m_render_resolution = render_resolution;
        }

        void set_target_resolution(const vk::Extent2D& target_resolution)
        {
            m_target_resolution = target_resolution;
        }

        const std::shared_ptr<RenderGraph::RenderPath>& get_render_path() const
        {
            return m_render_path;
        }

    private:
        vk::Extent2D m_render_resolution;

        vk::Extent2D m_target_resolution;

        const sdvk::Context& m_context;
        const sdvk::Swapchain& m_swapchain;
        std::shared_ptr<sd::Scene> m_selected_scene;
        std::shared_ptr<RenderGraph::RenderPath> m_render_path;
    };
}