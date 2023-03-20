#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Window/Window.hpp>

namespace sd
{
    class GUI
    {
    public:
        GUI(sdvk::Context const& context, sdvk::Swapchain const& swapchain, sd::Window const& window)
        {
            IMGUI_CHECKVERSION();

            m_window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+Vulkan example", nullptr, nullptr);
            auto r = glfwCreateWindowSurface(static_cast<VkInstance>(context.instance()), m_window, nullptr, &m_surface);

            vk::DescriptorPoolSize pool_sizes[] =
            {
                { vk::DescriptorType::eSampler, 1000 },
                { vk::DescriptorType::eCombinedImageSampler, 1000 },
                { vk::DescriptorType::eSampledImage, 1000 },
                { vk::DescriptorType::eStorageImage, 1000 },
                { vk::DescriptorType::eUniformTexelBuffer, 1000 },
                { vk::DescriptorType::eStorageTexelBuffer, 1000 },
                { vk::DescriptorType::eUniformBuffer, 1000 },
                { vk::DescriptorType::eStorageBuffer, 1000 },
                { vk::DescriptorType::eUniformBufferDynamic, 1000 },
                { vk::DescriptorType::eStorageBufferDynamic, 1000 },
                { vk::DescriptorType::eInputAttachment, 1000 }
            };

            vk::DescriptorPoolCreateInfo create_info;
            create_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
            create_info.setPPoolSizes(pool_sizes);
            create_info.setPoolSizeCount(std::size(pool_sizes));
            create_info.setMaxSets(1000);

            auto result = context.device().createDescriptorPool(&create_info, nullptr, &m_imgui_descriptor_pool);

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance        = context.instance();
            init_info.PhysicalDevice  = context.physical_device();
            init_info.Device          = context.device();
            init_info.Queue           = context.q_graphics().queue;
            init_info.QueueFamily     = context.q_graphics().index;
            init_info.PipelineCache   = nullptr;
            init_info.DescriptorPool  = m_imgui_descriptor_pool;
            init_info.ImageCount      = 2;
            init_info.MinImageCount   = 2;
            init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator       = nullptr;
            init_info.CheckVkResultFn = nullptr;

            ImGui_ImplVulkanH_Window* wd = &m_imgui_wd;
            wd->Surface = m_surface;
            wd->SurfaceFormat = swapchain.surface_format();
            wd->PresentMode = VK_PRESENT_MODE_FIFO_KHR;
            ImGui_ImplVulkanH_CreateOrResizeWindow(context.instance(),
                                                   context.physical_device(),
                                                   context.device(),
                                                   wd,
                                                   context.q_graphics().index,
                                                   nullptr,
                                                   swapchain.extent().width,
                                                   swapchain.extent().height,
                                                   swapchain.image_count());

            ImGui::CreateContext();
            ImGui_ImplGlfw_InitForVulkan(m_window, true);
            ImGui_ImplVulkan_Init(&init_info, m_imgui_wd.RenderPass);
        }

    private:
        GLFWwindow* m_window { nullptr };
        VkSurfaceKHR m_surface { VK_NULL_HANDLE };
        ImGui_ImplVulkanH_Window m_imgui_wd        {};
        vk::DescriptorPool m_imgui_descriptor_pool { VK_NULL_HANDLE };
    };
}