#include "Application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Vulkan/ContextBuilder.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Image/DepthBuffer.hpp>
#include <Vulkan/Presentation/SwapchainBuilder.hpp>
#include <Vulkan/Rendering/Mesh.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>

uint32_t g_current_frame = 0;
std::unique_ptr<sdvk::Mesh> g_mesh;
std::unique_ptr<sdvk::DepthBuffer> g_depth;
std::unique_ptr<sdvk::RenderPass> g_render_pass;
sdvk::Pipeline g_pipeline;
std::array<vk::ClearValue, 2> g_clear_values;
std::array<vk::Framebuffer, 2> g_framebuffers;

struct PushConstantBlock
{
    glm::vec4 color;
    glm::mat4 view, proj;
};

namespace sd
{
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
                .add_device_extensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_4_EXTENSION_NAME })
                .add_raytracing_extensions(true)
                .create_context();

        m_command_buffers = std::make_unique<sdvk::CommandBuffers>(8, *m_context);

        m_swapchain = sdvk::SwapchainBuilder(*m_window, *m_context)
            .with_defaults()
            .create();

        /*
        vk::Sampler test_sampler = sdvk::SamplerBuilder()
            .with_anisotropy(m_context->device_properties().limits.maxSamplerAnisotropy)
            .create(m_context->device());

        auto test_image = sdvk::Image::Builder()
            .with_extent({ 128, 128 })
            .create(*m_context);

        std::vector<uint32_t> indices = {0, 0, 0};
        auto test_index_buffer = sdvk::Buffer::Builder()
            .with_name("Test Index Buffer")
            .with_size(sizeof(uint32_t) * indices.size())
            .as_index_buffer()
            .create_with_data(indices.data(), *m_command_buffers, *m_context);

        std::vector<sd::VertexData> vertices = {{{0, 0, 0}, {1, 1, 1}, {0, 0}}};
        auto test_vertex_buffer = sdvk::Buffer::Builder()
            .with_name("Test Vertex Buffer")
            .with_size(sizeof(sd::VertexData) * vertices.size())
            .as_vertex_buffer()
            .create_with_data(vertices.data(), *m_command_buffers, *m_context);

        TestUniform uniform {};
        auto test_uniform_buffer = sdvk::Buffer::Builder()
            .with_name("Test Uniform Buffer")
            .with_size(sizeof(TestUniform))
            .as_uniform_buffer()
            .create_with_data(&uniform, *m_command_buffers, *m_context);
        */

        g_mesh = std::make_unique<sdvk::Mesh>(new primitives::Cube(), *m_command_buffers, *m_context);
        g_depth = std::make_unique<sdvk::DepthBuffer>(m_swapchain->extent(), *m_context);
        g_render_pass = std::make_unique<sdvk::RenderPass>(m_swapchain->format(), g_depth->format(), *m_context);
        g_pipeline = sdvk::PipelineBuilder(*m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantBlock) })
            .create_pipeline_layout()
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader("sdtest.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("sdtest.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(*g_render_pass);

        g_clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        g_clear_values[1].setDepthStencil({ 1.0f, 0 });

        std::array<vk::ImageView, 2> attachments = { m_swapchain->view(0), g_depth->view() };

        vk::FramebufferCreateInfo create_info;
        create_info.setRenderPass(g_render_pass->handle());
        create_info.setAttachmentCount(attachments.size());
        create_info.setPAttachments(attachments.data());
        create_info.setWidth(m_swapchain->extent().width);
        create_info.setHeight(m_swapchain->extent().height);
        create_info.setLayers(1);

        vk::Result result;

        for (size_t i = 0; i < 2; i++)
        {
            attachments[0] = m_swapchain->view(i);
            result = m_context->device().createFramebuffer(&create_info, nullptr, &g_framebuffers[i]);
        }
    }

    void Application::run()
    {
        m_window->while_open([&](){
            auto acquired_frame = m_swapchain->acquire_frame(g_current_frame);

            auto command_buffer = m_command_buffers->begin(g_current_frame);
            {
                vk::RenderPassBeginInfo begin_info;
                begin_info.setRenderPass(g_render_pass->handle());
                begin_info.setRenderArea({{0,0}, m_swapchain->extent()});
                begin_info.setClearValueCount(g_clear_values.size());
                begin_info.setPClearValues(g_clear_values.data());
                begin_info.setFramebuffer(g_framebuffers[g_current_frame]);
                command_buffer.beginRenderPass(&begin_info, vk::SubpassContents::eInline);

                auto viewport = m_swapchain->make_viewport();
                auto scissor = m_swapchain->make_scissor();
                command_buffer.setViewport(0, 1, &viewport);
                command_buffer.setScissor(0, 1, &scissor);

                PushConstantBlock pc_data {
                    .color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    .view = glm::lookAt(glm::vec3{5.0f, 5.0f, 5.0f}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0}),
                    .proj = glm::perspective(glm::radians(45.0f), m_swapchain->aspect_ratio(), 0.1f, 1000.0f)
                };
                command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, g_pipeline.pipeline);
                command_buffer.pushConstants(g_pipeline.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantBlock), &pc_data);

                g_mesh->draw(command_buffer);

                command_buffer.endRenderPass();
            }
            command_buffer.end();

            m_swapchain->submit_and_present(g_current_frame, acquired_frame, command_buffer);

            g_current_frame = (g_current_frame + 1) % m_swapchain->image_count();

            m_context->device().waitIdle();
        });
    }
}