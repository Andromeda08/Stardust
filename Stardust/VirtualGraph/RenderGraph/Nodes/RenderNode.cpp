#include "RenderNode.hpp"
#include <array>
#include <stdexcept>
#include <Application/Application.hpp>
#include <Nebula/Image.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>

namespace Nebula::RenderGraph
{
    std::vector<ResourceSpecification> RenderNode::s_resource_specs = {
        { "Objects", ResourceRole::eInput, ResourceType::eObjects },
        { "Camera", ResourceRole::eInput, ResourceType::eCamera },
        { "TLAS", ResourceRole::eInput, ResourceType::eTlas },
        { "G-Buffer", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Render Image", ResourceRole::eOutput, ResourceType::eImage, vk::Format::eR32G32B32A32Sfloat },
        { "Depth Image", ResourceRole::eOutput, ResourceType::eDepthImage },
    };

    RenderNode::RenderNode(const sdvk::Context& context)
    : m_context(context)
    {
    }

    void RenderNode::execute(const vk::CommandBuffer& command_buffer)
    {
        return;
    }

    void RenderNode::_initialize_renderer()
    {
        m_renderer.render_resolution = sd::Application::s_extent.vk_ext();
        m_renderer.frames_in_flight = sd::Application::s_max_frames_in_flight;

        /* auto depth_image = Nebula::Image::make_depth_image(m_context);
        m_resources.insert({
            "Depth Image",
            std::make_shared<DepthImageResource>(depth_image)
        }); */

        const auto& g_buffer_resource = m_resources["G-Buffer"];
        auto g_buffer = dynamic_cast<ImageResource&>(*g_buffer_resource).get_image();

        const auto& render_image_resource = m_resources["Render Image"];
        auto render_image = dynamic_cast<ImageResource&>(*render_image_resource).get_image();

        #pragma region RenderPass, Framebuffers

        m_renderer.renderpass = sdvk::RenderPass::Builder()
            .add_color_attachment(render_image->properties().format)
            .add_color_attachment(g_buffer->properties().format)
            // .add_color_attachment(depth_image->properties().format)
            .make_subpass()
            .create(m_context);

        std::array<vk::ImageView, 3> attachments = {
            render_image->image_view(),
            g_buffer->image_view(),
            // depth_image->image_view(),
        };

        vk::FramebufferCreateInfo create_info;
        create_info.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
        create_info.setPAttachments(attachments.data());
        create_info.setRenderPass(m_renderer.renderpass);
        create_info.setWidth(m_renderer.render_resolution.width);
        create_info.setHeight(m_renderer.render_resolution.height);
        create_info.setLayers(1);
        for (auto& fb : m_renderer.framebuffers)
        {
            auto result = m_context.device().createFramebuffer(&create_info, nullptr, &fb);
            if (result != vk::Result::eSuccess)
            {
                throw std::runtime_error("[Error] Failed to initialize rendering pipeline: Framebuffer creation failed.");
            }
        }

        #pragma endregion

        m_renderer.descriptor = Nebula::Descriptor::Builder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .acceleration_structure(1, vk::ShaderStageFlagBits::eFragment)
            .create(m_renderer.frames_in_flight, m_context);

        auto pipeline = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(RenderNodePushConstant) })
            .add_descriptor_set_layout(m_renderer.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(2)
            .add_attribute_descriptions({ sd::VertexData::attribute_descriptions() })
            .add_binding_descriptions({ sd::VertexData::binding_description() })
            .add_shader(RenderNodeRenderer::s_default_vertex_shader.data(), vk::ShaderStageFlagBits::eVertex)
            .add_shader(RenderNodeRenderer::s_default_fragment_shader.data(), vk::ShaderStageFlagBits::eFragment)
            .with_name("RenderNode")
            .create_graphics_pipeline(m_renderer.renderpass);

        m_renderer.pipeline = pipeline.pipeline;
        m_renderer.pipeline_layout = pipeline.pipeline_layout;

        for (auto& ub : m_renderer.ub_camera)
        {
            ub = sdvk::Buffer::Builder()
                .with_size(sizeof(sd::CameraUniformData))
                .as_uniform_buffer()
                .create(m_context);
        }

        m_renderer.clear_values[0].setColor(m_renderer.clear_color);
        m_renderer.clear_values[1].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
        m_renderer.clear_values[2].setDepthStencil({ 1.0f, 0 });
    }

    bool RenderNode::_validate_resource(const std::string& key, const std::shared_ptr<Resource>& resource)
    {
        bool valid_key = false;
        for (const auto& spec : s_resource_specs)
        {
            if (key == spec.name)
            {
                valid_key = true;
                break;
            }
        }

        if (!valid_key)
        {
            return false;
        }

        m_resources.insert_or_assign(key, resource);
        return true;
    }
}