#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <Rendering/RenderGraph/Node.hpp>
#include <Rendering/RenderGraph/ObjectPushConstant.hpp>
#include <Scene/Scene.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sd::rg
{
    class OffscreenRenderNode : public Node
    {
    public:
        OffscreenRenderNode(const sdvk::Context&   context,
                            const sdvk::Swapchain& swapchain,
                            const sd::Scene&       scene)
        : m_context(context), m_swapchain(swapchain), m_scene(scene), Node()
        {
            m_validators["Render"]  = Node::s_image_validator;
            m_validators["GBuffer"] = Node::s_image_validator;
            m_validators["Depth"]   = Node::s_depth_validator;

            create_resources();
        }

        void compile()
        {
            pre_compile();
            build_renderpass();
            build_pipeline();
            update_descriptors(0);

            m_is_ready = true;
        }

        void execute(const vk::CommandBuffer& cmd) override
        {
            auto commands = [&](const vk::CommandBuffer& cmd) {
                auto vp = m_swapchain.make_viewport();
                auto sc = m_swapchain.make_scissor();

                cmd.setViewport(0, 1, &vp);
                cmd.setScissor(0, 1, &sc);

                update_descriptors(0);

                for (const auto& obj : m_scene.objects())
                {
                    m_stage.obj_push_constant.model_matrix = obj.transform.model();
                    m_stage.obj_push_constant.color = obj.color;

                    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_stage.pipeline);
                    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_stage.pipeline_layout, 0, 1,
                                           &m_stage.descriptor->set(0), 0, nullptr);
                    cmd.pushConstants(m_stage.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(ObjectPushConstant), &m_stage.obj_push_constant);

                    obj.mesh->draw(cmd);
                }
            };

            sdvk::RenderPass::Execute()
                .with_render_pass(m_stage.renderpass)
                .with_render_area({{0, 0}, m_swapchain.extent()})
                .with_clear_values(m_stage.clear_values)
                .with_framebuffer(m_stage.framebuffer)
                .execute(cmd, commands);
        }

        void wire_input(const std::string& location, const std::string& from, const Node& src) override
        {
//            if (!m_validators[location](src[from]))
//            {
//                return;
//            }
//
//            m_resources[location] = src.get_resource(from);
        }

    private:
        void create_resources()
        {
            const vk::Extent2D extent = m_swapchain.extent();
            const auto format = vk::Format::eR32G32B32A32Sfloat;
            const auto usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
            const auto samples = vk::SampleCountFlagBits::e1;
            const auto memory = vk::MemoryPropertyFlagBits::eDeviceLocal;

            m_resources["Render"] = sdvk::Image::Builder()
                .with_extent(extent)
                .with_format(format)
                .with_sample_count(samples)
                .with_usage_flags(usage)
                .with_memory_property_flags(memory)
                .with_name("OffscreenRenderNode -> Render")
                .create(m_context);

            m_resources["GBuffer"] = sdvk::Image::Builder()
                .with_extent(extent)
                .with_format(format)
                .with_sample_count(samples)
                .with_usage_flags(usage)
                .with_memory_property_flags(memory)
                .with_name("OffscreenRenderNode -> G-Buffer")
                .create(m_context);

            m_resources["Depth"] = std::make_unique<sdvk::DepthBuffer>(extent, vk::SampleCountFlagBits::e1, m_context);

            m_stage.clear_values[0].setColor(m_params.clear_color);
            m_stage.clear_values[1].setColor(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f });
            m_stage.clear_values[2].setDepthStencil({ 1.0f, 0 });

            m_stage.camera_uniform_buffer = sdvk::Buffer::Builder()
                .with_size(sizeof(CameraUniformData))
                .as_uniform_buffer()
                .with_name("Camera UB")
                .create(m_context);

            auto camera_data = m_scene.camera_state();
            m_stage.camera_uniform_buffer->set_data(&camera_data, m_context.device());
        }

        // Checks for missing inputs and updates descriptors.
        void pre_compile()
        {
            bool has_missing_inputs = false;
            bool has_invalid_inputs = false;

            for (const auto& [k, v] : m_resources)
            {
                if (m_named_inputs.contains(k))
                {
                    if (m_validators[k](*v))
                    {
                        std::cerr << "Input \"" << k << "\" for node \"" << s_name << "\" is invalid!" << std::endl;
                        has_invalid_inputs = true;
                    }

                    if (v == nullptr)
                    {
                        std::cerr << "Node \"" << s_name << "\" is missing the input \"" << k << "\"!" << std::endl;
                        has_missing_inputs = true;
                    }
                }
            }

            if (has_missing_inputs || has_invalid_inputs)
            {
                std::stringstream ex_str;
                ex_str << "Cannot compile node \"" << s_name << "\"!";
                throw std::runtime_error(ex_str.str());
            }

            sdvk::DescriptorWrites(m_context.device(), *m_stage.descriptor)
                // TODO: Descriptor updates
                .commit();
        }

        void build_renderpass()
        {
            const auto extent = m_swapchain.extent();

            m_stage.renderpass = sdvk::RenderPass::Builder()
                .add_color_attachment(m_resources["Render"]->format())
                .add_color_attachment(m_resources["GBuffer"]->format())
                .set_depth_attachment(m_resources["Depth"]->format())
                .make_subpass()
                .with_name("OffscreenRenderNode -> RenderPass")
                .create(m_context);

            std::array<vk::ImageView, 3> attachments = {
                m_resources["Render"]->view(),
                m_resources["GBuffer"]->view(),
                m_resources["Depth"]->view()
            };

            vk::FramebufferCreateInfo create_info;
            create_info.setRenderPass(m_stage.renderpass);
            create_info.setAttachmentCount(static_cast<uint32_t>(attachments.size()));
            create_info.setPAttachments(attachments.data());
            create_info.setWidth(extent.width);
            create_info.setHeight(extent.height);
            create_info.setLayers(1);
            auto result = m_context.device()
                .createFramebuffer(&create_info, nullptr, &m_stage.framebuffer);
        }

        void build_pipeline()
        {
            m_stage.descriptor = sdvk::DescriptorBuilder()
                .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .accelerator(1, vk::ShaderStageFlagBits::eFragment)
                .with_name("OffscreenRenderNode -> Descriptor")
                .create(m_context.device(), 1);

            auto result = sdvk::PipelineBuilder(m_context)
                .add_push_constant({
                    vk::ShaderStageFlagBits::eVertex,
                    0, sizeof(ObjectPushConstant)
                })
                .add_descriptor_set_layout(m_stage.descriptor->layout())
                .create_pipeline_layout()
                .set_sample_count(vk::SampleCountFlagBits::e1)
                .set_attachment_count(2)
                .add_attribute_descriptions({ VertexData::attribute_descriptions() })
                .add_binding_descriptions({ VertexData::binding_description() })
                .add_shader("rg_offscreen_render.vert.spv", vk::ShaderStageFlagBits::eVertex)
                .add_shader("rg_offscreen_render.frag.spv", vk::ShaderStageFlagBits::eFragment)
                .with_name("OffscreenRenderNode -> Pipeline")
                .create_graphics_pipeline(m_stage.renderpass);

            m_stage.pipeline        = result.pipeline;
            m_stage.pipeline_layout = result.pipeline_layout;

            vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &m_scene.tlas().tlas() };
            sdvk::DescriptorWrites(m_context.device(), *m_stage.descriptor)
                .uniform_buffer(0, 0, { m_stage.camera_uniform_buffer->buffer(), 0, sizeof(CameraUniformData) })
                .acceleration_structure(0, 1, as_info)
                .commit();
        }

        void update_descriptors(uint32_t set)
        {
            auto camera_data = m_scene.camera_state();
            m_stage.camera_uniform_buffer->set_data(&camera_data, m_context.device());
        }

        struct NodePushConstant {
            bool with_shadows {true};
        };

        struct {
            bool with_shadows = true;
            std::array<float, 4> clear_color = { 0.1f, 0.1f, 0.1f, 1.0f };
        } m_params;

        struct {
            vk::Pipeline                      pipeline;
            vk::PipelineLayout                pipeline_layout;
            vk::RenderPass                    renderpass;
            vk::Framebuffer                   framebuffer;
            std::array<vk::ClearValue, 3>     clear_values;
            ObjectPushConstant                obj_push_constant;
            NodePushConstant                  node_push_constant;
            std::unique_ptr<sdvk::Descriptor> descriptor;
            std::unique_ptr<sdvk::Buffer>     camera_uniform_buffer;
        } m_stage;

        bool m_is_ready {false};

        const sdvk::Context&   m_context;
        const sdvk::Swapchain& m_swapchain;

        const sd::Scene&                                              m_scene;
        std::map<std::string, std::shared_ptr<resource_t>>            m_resources;
        std::map<std::string, std::function<bool(const resource_t&)>> m_validators;

        const std::set<std::string> m_named_inputs    = {};
        const std::set<std::string> m_named_rt_inputs = {"TLAS"};
        const std::set<std::string> m_named_outputs   = {"Render", "GBuffer", "Depth"};
        static constexpr std::string_view s_name      = "Offscreen Render";
    };
}