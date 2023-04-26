#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Barrier.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>
#include <Window/Window.hpp>

namespace sd
{
    class GUI
    {
    public:
        struct ImGuiPushConstant {
            glm::vec2 scale {};
            glm::vec2 translate {};
        } m_push_constant;

        GUI(sdvk::CommandBuffers const& command_buffers, sdvk::Context const& context)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui::StyleColorsDark();

            #pragma region Window & Surface & Swapchain
            m_window = std::make_unique<Window>(WindowOptions {
                .resolution = { 1280, 720 },
                .title = "ImGui Window",
                .fullscreen = false,
            });

            auto surface_result = glfwCreateWindowSurface(context.instance(),
                                                          m_window->handle(),
                                                          nullptr,
                                                          &m_surface);

            m_swapchain = sdvk::SwapchainBuilder(*m_window, context)
                    .with_defaults()
                    .create();
            #pragma endregion

            VkDescriptorPoolSize pool_sizes[] =
                    {
                            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                    };
            vk::DescriptorPoolCreateInfo ci;
            ci.setMaxSets(1000);


            vk::PipelineCacheCreateInfo pipeline_cache_create_info = {};
            auto pc_result = context.device().createPipelineCache(&pipeline_cache_create_info, nullptr, &m_pipeline_cache);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance        = context.instance();
            init_info.PhysicalDevice  = context.physical_device();
            init_info.Device          = context.device();
            init_info.Queue           = context.q_graphics().queue;
            init_info.QueueFamily     = context.q_graphics().index;
            init_info.PipelineCache   = m_pipeline_cache;
            init_info.DescriptorPool  = m_pool;
            init_info.ImageCount      = 2;
            init_info.MinImageCount   = 2;
            init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator       = nullptr;
            init_info.CheckVkResultFn = nullptr;
        }

    private:
        std::unique_ptr<Window> m_window;
        VkSurfaceKHR            m_surface { VK_NULL_HANDLE };

        std::unique_ptr<sdvk::Swapchain> m_swapchain;

        std::unique_ptr<sdvk::Image> m_font_image;
        vk::Sampler                  m_font_sampler;

        vk::DescriptorPool m_pool;

        std::unique_ptr<sdvk::Descriptor> m_descriptor;

        vk::RenderPass     m_renderpass;
        vk::Pipeline       m_pipeline;
        vk::PipelineCache  m_pipeline_cache;
        vk::PipelineLayout m_pipeline_layout;

    };
}