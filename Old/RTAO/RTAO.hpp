#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Utility/Clock.hpp>
#include <vk/Image.hpp>
#include <vk/Sampler.hpp>
#include <vk/Device/Device.hpp>
#include <vk/Renderpass/RenderPassBuilder.hpp>
#include <vk/Pipelines/PipelineBuilder.hpp>
#include <rt/RtAccelerator.hpp>

class RTAO
{
    struct Object
    {
        re::Mesh&   mesh;
        glm::mat4   transform;
        glm::vec4   color;
        std::string name;
    };
    struct CameraUniformData
    {
        glm::mat4 view, proj, view_inverse, proj_inverse;
        glm::vec4 eye;
    };
    struct PushConstantData
    {
        glm::vec4 time;
        glm::mat4 model;
        glm::vec4 color;
    };
    struct Descriptor {
        std::unique_ptr<DescriptorSets> sets;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayout layout;
    };
    using CameraUB = re::UniformBuffer<CameraUniformData>;

public:
    RTAO(const CommandBuffers& command_buffers)
    : m_cmds(command_buffers), m_device(command_buffers.device())
    {
        create_offscreen_render();
        create_post_descriptors();
        create_post_pipeline();
        create_objects();
    }

private:
    void add_object(const std::string &mesh_id, const glm::mat4 &model, const glm::vec4 &color, const std::string &name)
    {
        Object obj = {
            .mesh = *m_meshes[mesh_id],
            .transform = model,
            .color = color,
            .name = name
        };
        m_objects.push_back(obj);
    }

    void create_objects()
    {
        m_meshes["sphere"] = std::make_shared<re::Mesh>(new SphereGeometry(1.0f, glm::vec3(0.5f), 120), m_cmds);
        m_meshes["cube"] = std::make_shared<re::Mesh>(new CubeGeometry(), m_cmds);

        add_object("cube", Math::model(glm::vec3(0.0f), glm::vec3(25, 0.05f, 25)), glm::vec4(0.5f), "Plane");
        add_object("cube", Math::model(glm::vec3(-5, 1.0f, -5)), glm::vec4(0.7f, 0.1f, 0.9f, 1.0f), "Cube1");
        add_object("cube", Math::model(glm::vec3(-7, 1.0f, 3), glm::vec3(1.5f)), glm::vec4(0.2f, 0.1f, 0.9f, 1.0f), "Cube2");
        add_object("cube", Math::model(glm::vec3(2, 1.0f, 8)), glm::vec4(0.1f, 0.7f, 0.9f, 1.0f), "Cube3");
        add_object("sphere", Math::model(glm::vec3(5)), glm::vec4(1.0f), "Light");
    }

    void create_offscreen_render()
    {
        const auto usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
        const auto mprop = vk::MemoryPropertyFlagBits::eDeviceLocal;

        m_samplers.resize(4);
        for (uint32_t i = 0; i < 4; i++)
        {
            m_samplers[i] = std::make_unique<re::Sampler>(m_device);
        }

        // Required images: Color, gBuffer (rgba32), AO result (r32), Depth
        {
            m_colorBuffer = std::make_unique<re::Image>(m_size, m_offscreenColorFormat, vk::ImageTiling::eOptimal,
                                                        usage, mprop, vk::ImageAspectFlagBits::eColor, m_cmds);

            vk::DebugUtilsObjectNameInfoEXT s;
            s.setObjectHandle((uint64_t) static_cast<VkImage>(m_colorBuffer->image()));
            s.setObjectType(vk::ObjectType::eImage);
            s.setPObjectName("[RTAO] Offscreen");
            m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
        }

        {
            m_gBuffer = std::make_unique<re::Image>(m_size, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                                                    usage, mprop, vk::ImageAspectFlagBits::eColor, m_cmds);

            vk::DebugUtilsObjectNameInfoEXT s;
            s.setObjectHandle((uint64_t) static_cast<VkImage>(m_gBuffer->image()));
            s.setObjectType(vk::ObjectType::eImage);
            s.setPObjectName("[RTAO] G-Buffer");
            m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
        }

        {
            m_aoBuffer = std::make_unique<re::Image>(m_size, m_offscreenColorFormat, vk::ImageTiling::eOptimal,
                                                     usage, mprop, vk::ImageAspectFlagBits::eColor, m_cmds);

            vk::DebugUtilsObjectNameInfoEXT s;
            s.setObjectHandle((uint64_t) static_cast<VkImage>(m_aoBuffer->image()));
            s.setObjectType(vk::ObjectType::eImage);
            s.setPObjectName("[RTAO] AO Buffer");
            m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
        }

        {
            m_depthBuffer = std::make_unique<re::Image>(m_size, m_offscreenDepthFormat, vk::ImageTiling::eOptimal,
                                                        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                                                        mprop, vk::ImageAspectFlagBits::eDepth, m_cmds);

            vk::DebugUtilsObjectNameInfoEXT s;
            s.setObjectHandle((uint64_t) static_cast<VkImage>(m_depthBuffer->image()));
            s.setObjectType(vk::ObjectType::eImage);
            s.setPObjectName("[RTAO] Depth");
            m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
        }

        {
            auto cmd = m_cmds.begin_single_time();
            re::Image::set_layout(cmd, m_colorBuffer->image(),
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                                  { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            re::Image::set_layout(cmd, m_gBuffer->image(),
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                                  { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            re::Image::set_layout(cmd, m_aoBuffer->image(),
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                                  { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
            re::Image::set_layout(cmd, m_depthBuffer->image(),
                                  vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
                                  { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
            m_cmds.end_single_time(cmd);
        }

        m_offscreenRenderPass = RenderPassBuilder(m_device)
            .add_color_attachment(m_offscreenColorFormat, vk::ImageLayout::eGeneral)
            .add_color_attachment(m_offscreenColorFormat, vk::ImageLayout::eGeneral)
            .set_depth_attachment(m_offscreenDepthFormat, vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .make_subpass_description(vk::PipelineBindPoint::eGraphics)
            .make_framebuffer(m_size)
            .add_framebuffer_attachment(*m_colorBuffer)
            .add_framebuffer_attachment(*m_gBuffer)
            .add_framebuffer_attachment(*m_depthBuffer)
            .add_subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                    {}, vk::AccessFlagBits::eColorAttachmentWrite)
            .create();

        vk::DebugUtilsObjectNameInfoEXT s;
        s.setObjectHandle((uint64_t) static_cast<VkRenderPass>(m_offscreenRenderPass.render_pass));
        s.setObjectType(vk::ObjectType::eRenderPass);
        s.setPObjectName("[RTAO] Offscreen RenderPass");
        m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());

        s.setObjectHandle((uint64_t) static_cast<VkFramebuffer>(m_offscreenRenderPass.framebuffer));
        s.setObjectType(vk::ObjectType::eFramebuffer);
        s.setPObjectName("[RTAO] Offscreen Framebuffer");
        m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
    }

    void create_post_descriptors()
    {
        m_postDescriptor.layout = DescriptorSetLayout()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_postDescriptor.bindings)
            .create(m_device);

        m_postDescriptor.sets = std::make_unique<DescriptorSets>(m_postDescriptor.bindings, m_postDescriptor.layout, m_device);

        vk::DebugUtilsObjectNameInfoEXT s;
        s.setObjectHandle((uint64_t) static_cast<VkDescriptorSetLayout>(m_postDescriptor.layout));
        s.setObjectType(vk::ObjectType::eDescriptorSetLayout);
        s.setPObjectName("[RTAO] Post DSL");
        m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
    }

    void create_post_pipeline()
    {
        m_postPipeline = PipelineBuilder(m_device)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) })
            .add_descriptor_set_layout(m_postDescriptor.layout)
            .create_pipeline_layout()
            .add_shader("passthrough.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("post.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .create_graphics_pipeline(m_offscreenRenderPass.render_pass);

        vk::DebugUtilsObjectNameInfoEXT s;
        s.setObjectHandle((uint64_t) static_cast<VkPipelineLayout>(m_postPipeline.pipeline_layout));
        s.setObjectType(vk::ObjectType::ePipelineLayout);
        s.setPObjectName("[RTAO] Post Pipeline Layout");
        m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());

        s.setObjectHandle((uint64_t) static_cast<VkPipeline>(m_postPipeline.pipeline));
        s.setObjectType(vk::ObjectType::ePipeline);
        s.setPObjectName("[RTAO] Post Pipeline");
        m_device.handle().setDebugUtilsObjectNameEXT(&s, m_device.dispatch());
    }

    std::unique_ptr<sd::Clock> m_clock;
    RtAccelerator m_accelerator;
    std::vector<std::unique_ptr<Camera>> m_cameras;
    std::vector<Object> m_objects;
    std::unordered_map<std::string, Pipeline> m_materials;
    std::unordered_map<std::string, std::shared_ptr<re::Mesh>> m_meshes;
    Descriptor m_descriptors;
    std::vector<std::unique_ptr<CameraUB>> m_uniform_camera;

    vk::Extent2D m_size = { 1920, 1080 };

    // Offscreen Render
    std::vector<std::unique_ptr<re::Sampler>> m_samplers;
    std::unique_ptr<re::Image> m_gBuffer;
    std::unique_ptr<re::Image> m_aoBuffer;
    std::unique_ptr<re::Image> m_depthBuffer;
    std::unique_ptr<re::Image> m_colorBuffer;

    vk::Format m_offscreenDepthFormat = vk::Format::eX8D24UnormPack32;
    vk::Format m_offscreenColorFormat = vk::Format::eR32G32B32A32Sfloat;

    RenderPass2 m_offscreenRenderPass;

    Descriptor m_postDescriptor;
    Pipeline m_postPipeline;

    const CommandBuffers& m_cmds;
    const Device& m_device;
};