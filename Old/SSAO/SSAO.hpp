#pragma once
#include <unordered_map>
#include <Scenes/Scene.hpp>
#include <vk/Texture.hpp>
#include <vk/Descriptors/DescriptorSets.hpp>
#include <vk/Descriptors/DescriptorSetLayout.hpp>
#include <vk/Descriptors/DescriptorWrites.hpp>
#include <vk/Renderpass/RenderPassBuilder.hpp>
#include <vk/Pipelines/PipelineBuilder.hpp>

class SSAO
{
public:
    SSAO(Swapchain& swapchain, CommandBuffers const& command_buffers, vk::Format depth_format)
    : m_swapchain(swapchain), m_command_buffers(command_buffers)
    {
        m_offscreen.position = std::make_unique<re::Image>(m_swapchain.extent(), vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal,
                                                           vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                           vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                           vk::ImageAspectFlagBits::eColor,
                                                           m_command_buffers);

        m_offscreen.normal = std::make_unique<re::Image>(m_swapchain.extent(), vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
                                                         vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         m_command_buffers);

        m_offscreen.albedo = std::make_unique<re::Image>(m_swapchain.extent(), vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
                                                         vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                         vk::ImageAspectFlagBits::eColor,
                                                         m_command_buffers);

        m_offscreen.depth = std::make_unique<re::Image>(m_swapchain.extent(), depth_format, vk::ImageTiling::eOptimal,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
                                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                         vk::ImageAspectFlagBits::eDepth,
                                                         m_command_buffers);

        m_ssao.color = std::make_unique<re::Image>(m_swapchain.extent(), vk::Format::eR8Unorm, vk::ImageTiling::eOptimal,
                                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                   vk::ImageAspectFlagBits::eColor,
                                                   m_command_buffers);

        m_blur.color = std::make_unique<re::Image>(m_swapchain.extent(), vk::Format::eR8Unorm, vk::ImageTiling::eOptimal,
                                                   vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                                                   vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                   vk::ImageAspectFlagBits::eColor,
                                                   m_command_buffers);

        m_textures.ssao_noise = std::make_unique<re::Texture>("mirza_vulkan.jpg", m_command_buffers);

        m_offscreen.renderpass = RenderPassBuilder(m_command_buffers.device())
            .add_color_attachment(vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eShaderReadOnlyOptimal)
            .add_color_attachment(vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eShaderReadOnlyOptimal)
            .add_color_attachment(vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eShaderReadOnlyOptimal)
            .set_depth_attachment(depth_format, vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal)
            .make_subpass_description(vk::PipelineBindPoint::eGraphics)
            .add_subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                                    vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                    vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eColorAttachmentWrite,
                                    vk::DependencyFlagBits::eByRegion)
            .add_subpass_dependency(0, VK_SUBPASS_EXTERNAL,
                                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
                                    vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                                    vk::DependencyFlagBits::eByRegion)
            .make_framebuffer(m_swapchain.extent())
            .add_framebuffer_attachment(*m_offscreen.position)
            .add_framebuffer_attachment(*m_offscreen.normal)
            .add_framebuffer_attachment(*m_offscreen.albedo)
            .add_framebuffer_attachment(*m_offscreen.depth)
            .create();

        m_ssao.renderpass = RenderPassBuilder(m_command_buffers.device())
            .add_color_attachment(vk::Format::eR8Unorm, vk::ImageLayout::eShaderReadOnlyOptimal)
            .make_subpass_description(vk::PipelineBindPoint::eGraphics)
            .add_subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                                    vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                    vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                                    vk::DependencyFlagBits::eByRegion)
            .add_subpass_dependency(0, VK_SUBPASS_EXTERNAL,
                                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
                                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
                                    vk::DependencyFlagBits::eByRegion)
            .make_framebuffer(m_swapchain.extent())
            .add_framebuffer_attachment(*m_ssao.color)
            .create();

        m_blur.renderpass = RenderPassBuilder(m_command_buffers.device())
            .add_color_attachment(vk::Format::eR8Unorm, vk::ImageLayout::eShaderReadOnlyOptimal)
            .make_subpass_description(vk::PipelineBindPoint::eGraphics)
            .add_subpass_dependency(VK_SUBPASS_EXTERNAL, 0,
                                    vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                    vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
                                    vk::DependencyFlagBits::eByRegion)
            .add_subpass_dependency(0, VK_SUBPASS_EXTERNAL,
                                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
                                    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
                                    vk::DependencyFlagBits::eByRegion)
            .make_framebuffer(m_swapchain.extent())
            .add_framebuffer_attachment(*m_blur.color)
            .create();

        vk::SamplerCreateInfo sampler;
        sampler.setMagFilter(vk::Filter::eNearest);
        sampler.setMinFilter(vk::Filter::eNearest);
        sampler.setMipmapMode(vk::SamplerMipmapMode::eLinear);
        sampler.setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
        sampler.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
        sampler.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
        sampler.setMipLodBias(0.0f);
        sampler.setMaxAnisotropy(1.0f);
        sampler.setMinLod(0.0f);
        sampler.setMaxLod(1.0f);
        sampler.setBorderColor(vk::BorderColor::eFloatOpaqueWhite);
        auto result = m_command_buffers.device().handle().createSampler(&sampler, nullptr, &m_color_sampler);

        m_descriptors["offscreen"].layout = DescriptorSetLayout()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_descriptors["offscreen"].bindings)
            .create(m_command_buffers.device());

        m_descriptors["ssao"].layout = DescriptorSetLayout()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(2, vk::ShaderStageFlagBits::eFragment)
            .uniform_buffer(3, vk::ShaderStageFlagBits::eFragment)
            //.uniform_buffer(4, vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_descriptors["ssao"].bindings)
            .create(m_command_buffers.device());

        m_descriptors["blur"].layout = DescriptorSetLayout()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_descriptors["blur"].bindings)
            .create(m_command_buffers.device());

        m_descriptors["composition"].layout = DescriptorSetLayout()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(2, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(3, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(4, vk::ShaderStageFlagBits::eFragment)
            .uniform_buffer(5, vk::ShaderStageFlagBits::eFragment)
            .get_bindings(m_descriptors["composition"].bindings)
            .create(m_command_buffers.device());

        m_descriptors["offscreen"].sets = std::make_unique<DescriptorSets>(m_descriptors["offscreen"].bindings,
                                                                           m_descriptors["offscreen"].layout,
                                                                           m_command_buffers.device());

        m_descriptors["ssao"].sets = std::make_unique<DescriptorSets>(m_descriptors["ssao"].bindings,
                                                                      m_descriptors["ssao"].layout,
                                                                      m_command_buffers.device());

        m_descriptors["blur"].sets = std::make_unique<DescriptorSets>(m_descriptors["blur"].bindings,
                                                                           m_descriptors["blur"].layout,
                                                                           m_command_buffers.device());

        m_descriptors["composition"].sets = std::make_unique<DescriptorSets>(m_descriptors["composition"].bindings,
                                                                             m_descriptors["composition"].layout,
                                                                             m_command_buffers.device());

        m_ub_scene.resize(2);
        m_ub_ssao.resize(2);
        for (uint32_t i = 0; i < m_swapchain.image_count(); i++)
        {
            m_ub_scene[i] = std::make_unique<UB_Scene>(m_command_buffers);
            m_ub_ssao[i] = std::make_unique<UB_SSAO>(m_command_buffers);

            DescriptorWrites(m_command_buffers.device(), *m_descriptors["offscreen"].sets)
            .uniform_buffer(i, 0, { m_ub_scene[i]->buffer(), 0, sizeof(UBO_SceneParams) })
            .commit();


            DescriptorWrites(m_command_buffers.device(), *m_descriptors["ssao"].sets)
            .combined_image_sampler(i, 0, { m_color_sampler, m_offscreen.position->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 1, { m_color_sampler, m_offscreen.normal->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 2, { m_color_sampler, m_textures.ssao_noise->view(), vk::ImageLayout::eGeneral })
            .uniform_buffer(i, 3, { m_ub_ssao[i]->buffer(), 0, sizeof(UBO_SSAOParams) })
            .commit();

            DescriptorWrites(m_command_buffers.device(), *m_descriptors["blur"].sets)
            .combined_image_sampler(i, 0, { m_color_sampler, m_ssao.color->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .commit();

            DescriptorWrites(m_command_buffers.device(), *m_descriptors["composition"].sets)
            .combined_image_sampler(i, 0, { m_color_sampler, m_offscreen.position->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 1, { m_color_sampler, m_offscreen.normal->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 2, { m_color_sampler, m_offscreen.albedo->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 3, { m_color_sampler, m_ssao.color->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .combined_image_sampler(i, 4, { m_color_sampler, m_blur.color->view(), vk::ImageLayout::eShaderReadOnlyOptimal })
            .commit();
        }

        // TODO: Pipelines

        name_objects();
    }

    void name_objects()
    {
        const auto& device = m_command_buffers.device();

        vk::DebugUtilsObjectNameInfoEXT name;
        name.setObjectHandle((uint64_t) static_cast<VkRenderPass>(m_offscreen.renderpass.render_pass));
        name.setObjectType(vk::ObjectType::eRenderPass);
        name.setPObjectName("[SSAO] Offscreen RenderPass");
        auto result = device.handle().setDebugUtilsObjectNameEXT(&name, device.dispatch());
        
        name.setObjectHandle((uint64_t) static_cast<VkRenderPass>(m_ssao.renderpass.render_pass));
        name.setObjectType(vk::ObjectType::eRenderPass);
        name.setPObjectName("[SSAO] SSAO RenderPass");
        result = device.handle().setDebugUtilsObjectNameEXT(&name, device.dispatch());

        name.setObjectHandle((uint64_t) static_cast<VkRenderPass>(m_blur.renderpass.render_pass));
        name.setObjectType(vk::ObjectType::eRenderPass);
        name.setPObjectName("[SSAO] SSAO Blur RenderPass");
        result = device.handle().setDebugUtilsObjectNameEXT(&name, device.dispatch());
    }

    struct {
        std::unique_ptr<re::Texture> ssao_noise;
    } m_textures;

    struct Descriptors
    {
        std::unique_ptr<DescriptorSets> sets;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        vk::DescriptorSetLayout layout;
    };

    struct UBO_SceneParams
    {
        glm::mat4 projection, view;
        float near = 0.1f;
        float far  = 1000.0f;
    } m_ubo_scene;

    struct UBO_SSAOParams
    {
        glm::mat4 projection;
        int32_t ssao = true;
        int32_t blur = true;
    } m_ubo_ssao;

    struct OffscreenPass
    {
        RenderPass2 renderpass;
        std::unique_ptr<re::Image> position, normal, albedo, depth;
    } m_offscreen;

    struct SSAOPass
    {
        RenderPass2 renderpass;
        std::unique_ptr<re::Image> color;
    } m_ssao, m_blur;

    using UB_Scene = re::UniformBuffer<UBO_SceneParams>;
    using UB_SSAO = re::UniformBuffer<UBO_SSAOParams>;

    // offscreen / gbuffer | ssao | ssao blur | composition
    std::unordered_map<std::string, Pipeline>    m_pipelines;
    std::unordered_map<std::string, Descriptors> m_descriptors;
    std::vector<std::unique_ptr<UB_Scene>> m_ub_scene;
    std::vector<std::unique_ptr<UB_SSAO>> m_ub_ssao;

    vk::Sampler m_color_sampler;

    const CommandBuffers& m_command_buffers;
    Swapchain& m_swapchain;
};