#pragma once

#include <chrono>
#include <memory>
#include <Application/ApplicationOptions.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Editor/GraphEditor.hpp>
#include <Window/Window.hpp>

namespace sd
{
    class Application
    {
    public:
        explicit Application(ApplicationOptions const& options);

        void run();

    public:
        static constexpr uint32_t s_max_frames_in_flight {2};
        static constexpr bool s_imgui_enabled { true };
        static uint32_t s_current_frame;
        static sd::Extent s_extent;

    public:
        static std::tuple<float, float> get_ui_scale();

    private:
        void init_imgui();

        static std::tuple<float, float> get_ui_scale(const Extent& resolution);

        vk::DescriptorPool m_pool;
        vk::PipelineCache m_pipeline_cache { nullptr };
        vk::RenderPass m_renderpass;
        std::array<vk::Framebuffer, 2> m_fbos;

    private:
        ApplicationOptions m_options;

        std::shared_ptr<Nebula::RenderGraph::RenderGraphContext> m_rgctx;
        std::shared_ptr<Nebula::RenderGraph::Editor::GraphEditor> m_ge;

        std::unique_ptr<sd::Window> m_window;
        std::unique_ptr<sdvk::Context> m_context;
        std::unique_ptr<sdvk::CommandBuffers> m_command_buffers;
        std::unique_ptr<sdvk::Swapchain> m_swapchain;
        uint32_t m_current_frame = 0;

        std::chrono::high_resolution_clock::time_point m_launch_time = std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::time_point m_previous_time;
    };
}
