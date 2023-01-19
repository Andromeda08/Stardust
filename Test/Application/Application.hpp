#pragma once

#include <memory>
#include <Vulkan/Context.hpp>
#include "Vulkan/CommandBuffers.hpp"
#include <Vulkan/Presentation/Swapchain.hpp>
#include <Window/Window.hpp>
#include "ApplicationOptions.hpp"

namespace sd
{
    class Application
    {
    public:
        explicit Application(ApplicationOptions const& options);

        void run();

    private:
        ApplicationOptions m_options;

        std::unique_ptr<sd::Window> m_window;
        std::unique_ptr<sdvk::Context> m_context;
        std::unique_ptr<sdvk::CommandBuffers> m_command_buffers;
        std::unique_ptr<sdvk::Swapchain> m_swapchain;
    };
}
