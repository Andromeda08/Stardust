#pragma once

#include <memory>
#include <Scene/Scene.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>
#include <Window/Window.hpp>
#include "ApplicationOptions.hpp"
#include "GUI.hpp"

namespace sd
{
    class Application
    {
    public:
        explicit Application(ApplicationOptions const& options);

        void run();

    public:
        static constexpr uint32_t s_max_frames_in_flight {2};
        static uint32_t s_current_frame;
        static sd::Extent s_extent;

    private:
        ApplicationOptions m_options;

        std::unique_ptr<sd::Window> m_window;
        std::unique_ptr<sdvk::Context> m_context;
        std::unique_ptr<sdvk::CommandBuffers> m_command_buffers;
        std::unique_ptr<sdvk::Swapchain> m_swapchain;
        std::unique_ptr<Scene> m_scene;
        std::unique_ptr<sd::GUI> m_gui;
        uint32_t m_current_frame = 0;
    };
}
