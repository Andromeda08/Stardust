#include "Application.hpp"

#include <Vulkan/ContextBuilder.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>

#include <RenderGraph/RenderGraph.hpp>
#include <RenderGraph/node/CompositionNode.hpp>
#include <RenderGraph/node/RTAONode.hpp>
#include <RenderGraph/node/OffscreenRenderNode.hpp>
#include <RenderGraph/node/SceneNode.hpp>
#include <Application/UI.hpp>

#include <Benchmarking.hpp>

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
                .set_preferred_format(vk::Format::eB8G8R8Srgb)
                .with_defaults()
                .create();

        //m_gui = std::make_unique<sd::GUI>(*m_context, *m_swapchain, *m_window);
        //g_ui = std::make_unique<ui::UI>(*m_command_buffers, *m_context);

        auto scene_init = bm::measure<std::chrono::milliseconds>([&](){
            //m_scene = std::make_unique<Scene>(*m_command_buffers, *m_context, *m_swapchain);
            g_rgs = std::make_shared<sd::rg::Scene>(*m_command_buffers, *m_context, *m_swapchain);
        });

#pragma region node testing
        auto connect_time = bm::measure<std::chrono::milliseconds>([&](){
            g_snode = std::make_unique<sd::rg::SceneNode>(g_rgs);
            g_rtaonode = std::make_unique<sd::rg::RTAONode>(*m_context, *m_command_buffers);
            g_osrnode = std::make_unique<sd::rg::OffscreenRenderNode>(*m_context, *m_command_buffers);
            g_cnode = std::make_unique<sd::rg::CompositionNode>(*m_command_buffers, *m_context, *m_swapchain);

            auto& cam_res = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[0]);
            auto& tlas_res = dynamic_cast<rg::AccelerationStructureResource&>(*g_snode->m_outputs[2]);

            auto& oscam_res = dynamic_cast<rg::CameraResource&>(*g_osrnode->m_inputs[1]);
            oscam_res.m_resource = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[0]).m_resource;

            auto& osobj_res = dynamic_cast<rg::ObjectsResource&>(*g_osrnode->m_inputs[0]);
            osobj_res.m_resource = dynamic_cast<rg::ObjectsResource&>(*g_snode->m_outputs[1]).m_resource;

            auto& ostlas_res = dynamic_cast<rg::AccelerationStructureResource&>(*g_osrnode->m_inputs[2]);
            ostlas_res.m_resource = dynamic_cast<rg::AccelerationStructureResource&>(*g_snode->m_outputs[2]).m_resource;

            // Wire G-Buffer into AO node
            auto& gbfr_res = dynamic_cast<rg::ImageResource&>(*g_rtaonode->m_inputs[0]);
            gbfr_res.m_resource = dynamic_cast<rg::ImageResource&>(*g_osrnode->m_outputs[1]).m_resource;

            // Camera into AO node
            auto& rtcam_res = dynamic_cast<rg::CameraResource&>(*g_rtaonode->m_inputs[1]);
            rtcam_res.m_resource = dynamic_cast<rg::CameraResource&>(*g_snode->m_outputs[0]).m_resource;

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
    }

    void Application::run()
    {
        m_window->while_open([&](){
            g_rgs->register_keybinds(m_window->handle());

            auto acquired_frame = m_swapchain->acquire_frame(s_current_frame);

            auto command_buffer = m_command_buffers->begin(s_current_frame);

            auto vp = m_swapchain->make_viewport();
            auto sc = m_swapchain->make_scissor();
            command_buffer.setViewport(0, 1, &vp);
            command_buffer.setScissor(0, 1, &sc);

            g_osrnode->execute(command_buffer);
            g_rtaonode->execute(command_buffer);
            g_cnode->execute(command_buffer);

            command_buffer.end();

            m_swapchain->submit_and_present(s_current_frame, acquired_frame, command_buffer);
            s_current_frame = (s_current_frame + 1) % m_swapchain->image_count();

            //ImGui_ImplVulkan_NewFrame();
            //ImGui_ImplGlfw_NewFrame();
            //ImGui::NewFrame();
            //bool show = true;
            //ImGui::ShowDemoWindow(&show);
            //ImGui::Render();

            m_context->device().waitIdle();
        });
    }
}
