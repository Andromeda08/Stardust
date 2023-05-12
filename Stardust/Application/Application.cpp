#include "Application.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>
#include <imnodes.h>

#include <Benchmarking.hpp>
#include <Vulkan/ContextBuilder.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>

#include <RenderGraph/Scene.hpp>
#include <RenderGraph/node/CompositionNode.hpp>
#include <RenderGraph/node/RTAONode.hpp>
#include <RenderGraph/node/OffscreenRenderNode.hpp>
#include <RenderGraph/node/SceneNode.hpp>
#include <RenderGraph/res/ImageResource.hpp>
#include <RenderGraph/res/AccelerationStructureResource.hpp>
#include <RenderGraph/res/CameraResource.hpp>
#include <RenderGraph/res/ObjectsResource.hpp>

std::shared_ptr<sd::rg::Scene> g_rgs;
std::unique_ptr<sd::rg::RTAONode> g_rtaonode;
std::unique_ptr<sd::rg::OffscreenRenderNode> g_osrnode;
std::unique_ptr<sd::rg::SceneNode> g_snode;
std::unique_ptr<sd::rg::CompositionNode> g_cnode;

namespace sd
{
    uint32_t Application::s_current_frame = 0;

    Application::Application(const ApplicationOptions& options)
    : m_options(options)
    {
        m_window = std::make_unique<Window>(m_options.window_options);

        auto ctx_init = bm::measure<std::chrono::milliseconds>([&](){
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
        });

        m_command_buffers = std::make_unique<sdvk::CommandBuffers>(8, *m_context);

        m_swapchain = sdvk::SwapchainBuilder(*m_window, *m_context)
                .with_defaults()
                .create();

        auto scene_init = bm::measure<std::chrono::milliseconds>([&](){
            g_rgs = std::make_shared<sd::rg::Scene>(*m_command_buffers, *m_context, *m_swapchain);
        });

        m_editor = std::make_shared<rg::RenderGraphEditor>(*m_command_buffers, *m_context, *m_swapchain, g_rgs);

        m_editor->_add_initial_nodes();

#pragma region node testing
        auto connect_time = bm::measure<std::chrono::milliseconds>([&](){
            g_snode = std::make_unique<sd::rg::SceneNode>(g_rgs);
            g_rtaonode = std::make_unique<sd::rg::RTAONode>(*m_context, *m_command_buffers);
            g_osrnode = std::make_unique<sd::rg::OffscreenRenderNode>(*m_context, *m_command_buffers);
            g_cnode = std::make_unique<sd::rg::CompositionNode>(*m_command_buffers, *m_context, *m_swapchain);

            auto& cam_res = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[1]);
            auto& tlas_res = dynamic_cast<rg::AccelerationStructureResource&>(*g_snode->m_outputs[2]);

            auto& oscam_res = dynamic_cast<rg::CameraResource&>(*g_osrnode->m_inputs[1]);
            oscam_res.m_resource = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[1]).m_resource;

            auto& osobj_res = dynamic_cast<rg::ObjectsResource&>(*g_osrnode->m_inputs[0]);
            osobj_res.m_resource = dynamic_cast<rg::ObjectsResource&>(*g_snode->m_outputs[0]).m_resource;

            auto& ostlas_res = dynamic_cast<rg::AccelerationStructureResource&>(*g_osrnode->m_inputs[2]);
            ostlas_res.m_resource = dynamic_cast<rg::AccelerationStructureResource&>(*g_snode->m_outputs[2]).m_resource;

            // Wire G-Buffer into AO node
            auto& gbfr_res = dynamic_cast<rg::ImageResource&>(*g_rtaonode->m_inputs[0]);
            gbfr_res.m_resource = dynamic_cast<rg::ImageResource&>(*g_osrnode->m_outputs[1]).m_resource;

            // Camera into AO node
            auto& rtcam_res = dynamic_cast<rg::CameraResource&>(*g_rtaonode->m_inputs[1]);
            rtcam_res.m_resource = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[1]).m_resource;

            // Wire TLAS into AO node
            auto& rttlas_res = dynamic_cast<rg::AccelerationStructureResource&>(*g_rtaonode->m_inputs[2]);
            rttlas_res.m_resource = dynamic_cast<rg::AccelerationStructureResource&>(*g_snode->m_outputs[2]).m_resource;

            auto& cimg_res = dynamic_cast<rg::ImageResource&>(*g_cnode->m_inputs[0]);
            cimg_res.m_resource = dynamic_cast<rg::ImageResource&>(*g_osrnode->m_outputs[0]).m_resource;

            auto& cao_res = dynamic_cast<rg::ImageResource&>(*g_cnode->m_inputs[1]);
            cao_res.m_resource = dynamic_cast<rg::ImageResource&>(*g_rtaonode->m_outputs[0]).m_resource;
        });

        auto compile_t1 = bm::measure<std::chrono::milliseconds>([&](){ g_osrnode->compile(); });
        auto compile_t2 = bm::measure<std::chrono::milliseconds>([&](){ g_rtaonode->compile(); });
        auto compile_t3 = bm::measure<std::chrono::milliseconds>([&](){ g_cnode->compile(); });
#pragma endregion

        std::cout
                << "Ctx init time   : " << ctx_init.count() << "ms\n"
                << "Scene init time : " << scene_init.count() << "ms\n"
                << "Connection time : " << connect_time.count() << "ms\n"
                << "Compile OSR     : " << compile_t1.count() << "ms\n"
                << "Compile RTAO    : " << compile_t2.count() << "ms\n"
                << "Compile Comp.   : " << compile_t3.count() << "ms\n"
                << std::endl;

        init_imgui();
    }

    void Application::run()
    {
        auto render_command = [&](){
            g_rgs->register_keybinds(m_window->handle());

            auto acquired_frame = m_swapchain->acquire_frame(s_current_frame);

            auto command_buffer = m_command_buffers->begin(s_current_frame);

            auto vp = m_swapchain->make_viewport();
            auto sc = m_swapchain->make_scissor();
            command_buffer.setViewport(0, 1, &vp);
            command_buffer.setScissor(0, 1, &sc);

            bool editor = m_editor->execute(command_buffer);

            if (!editor) {
                g_osrnode->execute(command_buffer);
                g_rtaonode->execute(command_buffer);
                g_cnode->execute(command_buffer);
            }

            std::array<vk::ClearValue, 1> clear_value;
            clear_value[0].color = std::array<float, 4>({ 0.f, 0.f, 0.f, 0.f });
            sdvk::RenderPass::Execute()
                .with_framebuffer(m_fbos[s_current_frame])
                .with_render_pass(m_renderpass)
                .with_clear_value(clear_value)
                .with_render_area({{0, 0}, m_swapchain->extent()})
                .execute(command_buffer, [&](const auto& cmd) {
                    if (sd::Application::s_imgui_enabled)
                    {
                        ImGuiIO& io = ImGui::GetIO();

                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        {
                            ImGui::Begin("Metrics");
                            ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
                            ImGui::End();

                            m_editor->draw();
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
        vk::Extent2D extent = m_swapchain->extent();
        std::vector<vk::ImageView> comp_attachments = { m_swapchain->view(0) };
        framebuffer_create_info.setRenderPass(m_renderpass);
        framebuffer_create_info.setAttachmentCount(comp_attachments.size());
        framebuffer_create_info.setPAttachments(comp_attachments.data());
        framebuffer_create_info.setWidth(extent.width);
        framebuffer_create_info.setHeight(extent.height);
        framebuffer_create_info.setLayers(1);
        for (int32_t i = 0; i < 2; i++)
        {
            comp_attachments[0] = m_swapchain->view(i);
            vk::Result result = m_context->device().createFramebuffer(&framebuffer_create_info, nullptr, &m_fbos[i]);
        }
        #pragma endregion


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

        vk::DescriptorPoolCreateInfo pool_info = {};
        pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        pool_info.setMaxSets(1000 * IM_ARRAYSIZE(pool_sizes));
        pool_info.setPPoolSizes(pool_sizes);
        pool_info.setPoolSizeCount((uint32_t) IM_ARRAYSIZE(pool_sizes));

        auto r = m_context->device().createDescriptorPool(&pool_info, nullptr, &m_pool);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        //io.FontGlobalScale = 1.0f;
        io.Fonts->AddFontFromFileTTF("JetBrainsMono-Regular.ttf", 13);

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        //style.ScaleAllSizes(1.25f);

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
}
