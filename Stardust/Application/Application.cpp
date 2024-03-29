#include "Application.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imnodes.h>
#include <Vulkan/ContextBuilder.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Scene/Scene.hpp>
#include <VirtualGraph/Builder/Builder.h>

std::shared_ptr<sd::Scene> g_rgs;

namespace sd
{
    uint32_t Application::s_current_frame = 0;

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
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
                VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                VK_EXT_MESH_SHADER_EXTENSION_NAME,
            })
            .add_raytracing_extensions(true)
            .create_context();

        m_command_buffers = std::make_unique<sdvk::CommandBuffers>(8, *m_context);

        m_swapchain = sdvk::SwapchainBuilder(*m_window, *m_context)
                .with_defaults()
                .set_preferred_color_space(vk::ColorSpaceKHR::eSrgbNonlinear)
                .set_preferred_format(vk::Format::eB8G8R8A8Unorm)
                .create();

        g_rgs = std::make_shared<Scene>(*m_command_buffers, *m_context);

        m_rgctx = std::make_shared<Nebula::RenderGraph::RenderGraphContext>(*m_command_buffers, *m_context, *m_swapchain);
        m_rgctx->set_scene(g_rgs);

        m_ge = std::make_shared<Nebula::RenderGraph::Editor::GraphEditor>(*m_rgctx);
        m_ge->set_scene(g_rgs);

        // Build initial graph
        const auto initial_graph = Nebula::RenderGraph::Builder::create_initial_graph(m_rgctx);
        m_rgctx->set_render_path(initial_graph.render_path);

        init_imgui();
    }

    void Application::run()
    {
        static auto convert_memory = [&](const uint64_t input_memory){
            std::tuple<float, std::string> result;

            static constexpr float kilobyte_coefficient = 1024.0f;
            static constexpr float megabyte_coefficient = kilobyte_coefficient * 1024.0f;
            static constexpr float gigabyte_coefficient = megabyte_coefficient * 1024.0f;

            auto memory = static_cast<float>(input_memory);
            if (memory < kilobyte_coefficient)
            {
                result = { memory, "B" };
            }
            else if (memory < megabyte_coefficient)
            {
                result = { memory / kilobyte_coefficient, "KB" };
            }
            else if (memory < gigabyte_coefficient)
            {
                result = { memory / megabyte_coefficient, "MB" };
            }
            else
            {
                result = { memory / gigabyte_coefficient, "GB" };
            }

            return result;
        };

        auto render_command = [&]{
            const ImGuiIO& _io = ImGui::GetIO();
            if (!_io.WantCaptureMouse)
            {
                g_rgs->mouse_handler(*m_window);
            }
            if (!_io.WantCaptureKeyboard)
            {
                g_rgs->key_handler(*m_window);
            }

            vk::PhysicalDeviceMemoryProperties2 mem_props;
            vk::PhysicalDeviceMemoryBudgetPropertiesEXT mem_budget;
            mem_props.pNext = &mem_budget;
            m_context->physical_device().getMemoryProperties2(&mem_props);

            uint64_t memory_usage {0};
            uint64_t memory_budget {0};
            for (int32_t i = 0; i < mem_props.memoryProperties.memoryHeapCount; i++)
            {
                memory_usage += mem_budget.heapUsage[i];
                memory_budget += mem_budget.heapBudget[i];
            }

            auto [ mu, mu_m] = convert_memory(memory_usage);
            auto [ mb, mb_m ] = convert_memory(memory_budget);

            const auto acquired_frame = m_swapchain->acquire_frame(s_current_frame);

            const auto command_buffer = m_command_buffers->begin(s_current_frame);

            const auto vp = m_swapchain->make_viewport();
            const auto sc = m_swapchain->make_scissor();
            command_buffer.setViewport(0, 1, &vp);
            command_buffer.setScissor(0, 1, &sc);

            m_rgctx->get_render_path()->execute(command_buffer);

            std::array<vk::ClearValue, 1> clear_value;
            clear_value[0].color = std::array<float, 4>({ 0.f, 0.f, 0.f, 0.f });
            sdvk::RenderPass::Execute()
                .with_framebuffer(m_fbos[s_current_frame])
                .with_render_pass(m_renderpass)
                .with_clear_value(clear_value)
                .with_render_area({{0, 0}, m_swapchain->extent()})
                .execute(command_buffer, [&](auto& cmd) {
                    if (s_imgui_enabled)
                    {
                        const ImGuiIO& io = ImGui::GetIO();

                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        {
                            ImGui::Begin("Metrics");
                            ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
                            ImGui::Text("Total Memory Usage: %.2f %s", mu, mu_m.c_str());
                            ImGui::Text("Available Memory Budget: %.2f %s", mb, mb_m.c_str());
                            ImGui::End();

                            m_ge->render();
                        }
                        ImGui::EndFrame();

                        ImGui::Render();
                        ImDrawData* main_draw_data = ImGui::GetDrawData();
                        ImGui_ImplVulkan_RenderDrawData(main_draw_data, cmd);
                    }
                });

            command_buffer.end();

            m_swapchain->submit_and_present(s_current_frame, acquired_frame, command_buffer);
            s_current_frame = (s_current_frame + 1) % m_swapchain->image_count();

            m_context->device().waitIdle();
        };

        m_window->while_open(render_command);
    }

    void Application::init_imgui()
    {
        m_renderpass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_swapchain->format(), vk::SampleCountFlagBits::e1, vk::ImageLayout::ePresentSrcKHR, vk::AttachmentLoadOp::eLoad)
            .make_subpass()
            .with_name("ImGui RenderPass")
            .create(*m_context);

        vk::FramebufferCreateInfo framebuffer_create_info;
        #pragma region framebuffer creation

        const vk::Extent2D extent = m_swapchain->extent();
        std::vector comp_attachments = { m_swapchain->view(0) };
        framebuffer_create_info.setRenderPass(m_renderpass);
        framebuffer_create_info.setAttachmentCount(comp_attachments.size());
        framebuffer_create_info.setPAttachments(comp_attachments.data());
        framebuffer_create_info.setWidth(extent.width);
        framebuffer_create_info.setHeight(extent.height);
        framebuffer_create_info.setLayers(1);
        for (int32_t i = 0; i < 2; i++)
        {
            comp_attachments[0] = m_swapchain->view(i);
            if (const vk::Result result = m_context->device().createFramebuffer(&framebuffer_create_info, nullptr, &m_fbos[i]);
                result != vk::Result::eSuccess)
            {
                throw std::runtime_error(std::format("[Error] Failed to create ImGui Framebuffer #{}", i));
            }
        }

        #pragma endregion

        const vk::DescriptorPoolSize pool_sizes[]
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

        vk::DescriptorPoolCreateInfo pool_info = {};
        pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        pool_info.setMaxSets(1000 * IM_ARRAYSIZE(pool_sizes));
        pool_info.setPPoolSizes(pool_sizes);
        pool_info.setPoolSizeCount(IM_ARRAYSIZE(pool_sizes));

        if (const vk::Result result = m_context->device().createDescriptorPool(&pool_info, nullptr, &m_pool);
            result != vk::Result::eSuccess)
        {
            throw std::runtime_error("[Error] Failed to create ImGui DescriptorPool");
        }

        auto [ui_scale, font_size] = get_ui_scale(s_extent);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = ui_scale;
        io.Fonts->AddFontFromFileTTF("JetBrainsMono-Regular.ttf", font_size);

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(ui_scale);

        ImGui_ImplGlfw_InitForVulkan(m_window->handle(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = m_context->instance();
        init_info.PhysicalDevice = m_context->physical_device();
        init_info.Device = m_context->device();
        init_info.QueueFamily = m_context->q_graphics().index;
        init_info.Queue = m_context->q_graphics().queue;
        init_info.PipelineCache = m_pipeline_cache;
        init_info.DescriptorPool = m_pool;
        init_info.Subpass = 0;
        init_info.ImageCount = 2;
        init_info.MinImageCount = 2;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info, m_renderpass);

        m_command_buffers->execute_single_time([&](const auto& cmd){
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        ImNodes::CreateContext();
        ImNodes::StyleColorsDark();
    }

    std::tuple<float, float> Application::get_ui_scale(const Extent& resolution)
    {
        if (resolution.width <= 1920 && resolution.height <= 1080)
        {
            return { 1.0f, 16.0f };
        }

        return { 1.5f, 18.0f };
    }

    std::tuple<float, float> Application::get_ui_scale()
    {
        return get_ui_scale(s_extent);
    }
}
