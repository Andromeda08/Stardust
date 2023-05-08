#include "CompositionNode.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Barrier.hpp>

namespace sd::rg
{
    CompositionNode::CompositionNode(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context,
                                     const sdvk::Swapchain& swapchain)
    : m_command_buffers(command_buffers)
    , m_context(context)
    , m_swapchain(swapchain)
    {
        _init_inputs();
    }

    void CompositionNode::execute(const vk::CommandBuffer& command_buffer)
    {
        uint32_t current_frame = Application::s_current_frame;

        static auto cmds = [&](const vk::CommandBuffer& cmd){
            float aspect_ratio = m_swapchain.aspect_ratio();
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_renderer.pipeline);
            cmd.pushConstants(m_renderer.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), &aspect_ratio);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_renderer.pipeline_layout,0, 1,
                                   &m_renderer.descriptor->set(current_frame),
                                   0, nullptr);

            cmd.draw(3, 1, 0, 0);

            if (sd::Application::s_imgui_enabled)
            {
                ImGuiIO& io = ImGui::GetIO();

                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                {
                    ImGui::Begin("ImGui test");
                    ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
                    ImGui::End();
                }

                ImGui::Render();
                ImDrawData* main_draw_data = ImGui::GetDrawData();
                ImGui_ImplVulkan_RenderDrawData(main_draw_data, cmd);
            }
        };

        auto& render_img = *dynamic_cast<ImageResource&>(*m_inputs[0]).m_resource;

        auto render_img_barrier = sdvk::ImageMemoryBarrier::Builder()
                .layout(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal)
                .access_mask(vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead)
                .create();

        auto renderpass = sdvk::RenderPass::Execute()
                .with_render_pass(m_renderer.render_pass)
                .with_render_area({{0, 0}, m_swapchain.extent()})
                .with_clear_values(m_renderer.clear_values)
                .with_framebuffer(m_renderer.sc_framebuffers[current_frame]);

        render_img_barrier.set_image(render_img.image());
        render_img_barrier.wrap(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, command_buffer,
                   [&](){ renderpass.execute(command_buffer, cmds); });
    }

    void CompositionNode::compile()
    {
        _check_features();
        _init_resources();
        for (int32_t i = 0; i < n_frames_in_flight; i++)
        {
            _update_descriptors(i);
        }
    }

    void CompositionNode::_init_inputs()
    {
        m_inputs.resize(2);
        m_inputs[0] = ImageResource::Builder()
                .accept_formats({ vk::Format::eR32G32B32A32Sfloat })
                .create();

        m_inputs[1] = ImageResource::Builder()
                .accept_formats({ vk::Format::eR32Sfloat })
                .create();
    }

    void CompositionNode::_init_resources()
    {
        m_renderer.sampler = sdvk::SamplerBuilder().create(m_context.device());
        m_renderer.descriptor = sdvk::DescriptorBuilder()
                .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
                .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
                .with_name("Composite")
                .create(m_context.device(), n_frames_in_flight);

        m_renderer.render_pass = sdvk::RenderPass::Builder()
                .add_color_attachment(m_swapchain.format(), vk::SampleCountFlagBits::e1, vk::ImageLayout::ePresentSrcKHR)
                .make_subpass()
                .with_name("Composite RenderPass")
                .create(m_context);

        m_renderer.clear_values[0].color = std::array<float, 4>{ 0.05f, 0.05f, 0.05f, 0.5f };

        vk::FramebufferCreateInfo framebuffer_create_info;
        #pragma region framebuffer creation
        vk::Extent2D extent = m_swapchain.extent();
        std::vector<vk::ImageView> comp_attachments = { m_swapchain.view(0) };
        framebuffer_create_info.setRenderPass(m_renderer.render_pass);
        framebuffer_create_info.setAttachmentCount(comp_attachments.size());
        framebuffer_create_info.setPAttachments(comp_attachments.data());
        framebuffer_create_info.setWidth(extent.width);
        framebuffer_create_info.setHeight(extent.height);
        framebuffer_create_info.setLayers(1);
        for (int32_t i = 0; i < n_frames_in_flight; i++)
        {
            comp_attachments[0] = m_swapchain.view(i);
            vk::Result result = m_context.device().createFramebuffer(&framebuffer_create_info, nullptr, &m_renderer.sc_framebuffers[i]);
        }
        #pragma endregion

        auto pipeline_result = sdvk::PipelineBuilder(m_context)
                .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) })
                .add_descriptor_set_layout(m_renderer.descriptor->layout())
                .create_pipeline_layout()
                .set_sample_count(vk::SampleCountFlagBits::e1)
                .add_shader(s_vertex_shader.data(), vk::ShaderStageFlagBits::eVertex)
                .add_shader(s_fragment_shader.data(), vk::ShaderStageFlagBits::eFragment)
                .set_cull_mode(vk::CullModeFlagBits::eNone)
                .with_name("Composite")
                .create_graphics_pipeline(m_renderer.render_pass);

        m_renderer.pipeline = pipeline_result.pipeline;
        m_renderer.pipeline_layout = pipeline_result.pipeline_layout;
    }

    void CompositionNode::_update_descriptors(uint32_t current_frame)
    {
        auto& render_img = *dynamic_cast<ImageResource&>(*m_inputs[0]).m_resource;
        auto& ao_img     = *dynamic_cast<ImageResource&>(*m_inputs[1]).m_resource;

        vk::DescriptorImageInfo offscreen = { m_renderer.sampler, render_img.view(), vk::ImageLayout::eShaderReadOnlyOptimal};
        vk::DescriptorImageInfo ao_buffer = { m_renderer.sampler, ao_img.view(), vk::ImageLayout::eShaderReadOnlyOptimal};

        sdvk::DescriptorWrites(m_context.device(), *m_renderer.descriptor)
            .combined_image_sampler(current_frame, 0, offscreen)
            .combined_image_sampler(current_frame, 1, ao_buffer)
            .commit();
    }

    void CompositionNode::_check_features()
    {
        static auto test_ao = [&](){
            auto resource = dynamic_cast<ImageResource&>(*m_inputs[1]);
            return resource.m_resource == nullptr;
        };

        m_features.ambient_occlusion = test_ao();
    }
}