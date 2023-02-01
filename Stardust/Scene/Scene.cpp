#include "Scene.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <Rendering/RTAO/RTAOKernel.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>

namespace sd
{
    Scene::Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context, const sdvk::Swapchain& swapchain)
    : m_command_buffers(command_buffers), m_context(context), m_swapchain(swapchain)
    {
        m_render_settings.ambient_occlusion.mode = AmbientOcclusionMode::eRTAO;

        auto extent = m_swapchain.extent();
        m_camera = std::make_unique<Camera>(glm::ivec2(extent.width, extent.height), glm::vec3(5.0f));

        #pragma region offscreen
        m_offscreen_render_target.g_buffer = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(vk::Format::eR32G32B32A32Sfloat)
            .with_sample_count(vk::SampleCountFlagBits::e1)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("G-Buffer")
            .create(m_context);

        m_offscreen_render_target.output = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(m_swapchain.format())
            .with_sample_count(vk::SampleCountFlagBits::e1)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("Offscreen render")
            .create(m_context);

        m_offscreen_render_target.depth_image = std::make_unique<sdvk::DepthBuffer>(m_swapchain.extent(), vk::SampleCountFlagBits::e1, m_context);

        m_offscreen_render_target.clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        m_offscreen_render_target.clear_values[1].setColor(std::array<float, 4>{ 0, 0, 0, 0 });
        m_offscreen_render_target.clear_values[2].setDepthStencil({ 1.0f, 0 });

        m_offscreen_render_target.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_offscreen_render_target.output->format(), vk::SampleCountFlagBits::e1)
            .add_color_attachment(m_offscreen_render_target.g_buffer->format(), vk::SampleCountFlagBits::e1)
            .set_depth_attachment(m_offscreen_render_target.depth_image->format(), vk::SampleCountFlagBits::e1)
            .make_subpass()
            .with_name("Offscreen RenderPass")
            .create(m_context);

        m_offscreen_render_target.descriptor = sdvk::DescriptorBuilder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .accelerator(1, vk::ShaderStageFlagBits::eFragment)
            .with_name("Offscreen")
            .create(context.device(), m_swapchain.image_count());

        m_uniform_camera.resize(m_swapchain.image_count());
        auto camera_data = m_camera->uniform_data();
        for (int32_t i = 0; i < m_uniform_camera.size(); i++)
        {
            m_uniform_camera[i] = sdvk::Buffer::Builder()
                .with_size(sizeof(CameraUniformData))
                .as_uniform_buffer()
                .with_name("Camera UB #" + std::to_string(i))
                .create(m_context);

            m_uniform_camera[i]->set_data(&camera_data, m_context.device());

            sdvk::DescriptorWrites(m_context.device(), *m_offscreen_render_target.descriptor)
                .uniform_buffer(i, 0, { m_uniform_camera[i]->buffer(), 0, sizeof(CameraUniformData) })
                .commit();
        }

        std::array<vk::ImageView, 3> os_attachments = {
            m_offscreen_render_target.output->view(),
            m_offscreen_render_target.g_buffer->view(),
            m_offscreen_render_target.depth_image->view()
        };

        vk::FramebufferCreateInfo fb_create_info;
        fb_create_info.setRenderPass(m_offscreen_render_target.render_pass);
        fb_create_info.setAttachmentCount(os_attachments.size());
        fb_create_info.setPAttachments(os_attachments.data());
        fb_create_info.setWidth(extent.width);
        fb_create_info.setHeight(extent.height);
        fb_create_info.setLayers(1);
        auto result = m_context.device().createFramebuffer(&fb_create_info, nullptr, &m_offscreen_render_target.framebuffer);

        m_pipelines["default"] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData) })
            .add_descriptor_set_layout(m_offscreen_render_target.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(2)
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader("rq_light.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rtao.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("Offscreen Graphics")
            .create_graphics_pipeline(m_offscreen_render_target.render_pass);
        #pragma endregion

        #pragma region composite
        m_composite_render_target.sampler = sdvk::SamplerBuilder().create(m_context.device());
        m_composite_render_target.descriptor = sdvk::DescriptorBuilder()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .with_name("Composite")
            .create(m_context.device(), 1);

        m_composite_render_target.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_swapchain.format(), vk::SampleCountFlagBits::e1, vk::ImageLayout::ePresentSrcKHR)
            .make_subpass()
            .with_name("Composite RenderPass")
            .create(m_context);

        std::vector<vk::ImageView> comp_attachments = { m_swapchain.view(0) };
        vk::FramebufferCreateInfo fb_create_info2;
        fb_create_info2.setRenderPass(m_composite_render_target.render_pass);
        fb_create_info2.setAttachmentCount(comp_attachments.size());
        fb_create_info2.setPAttachments(comp_attachments.data());
        fb_create_info2.setWidth(extent.width);
        fb_create_info2.setHeight(extent.height);
        fb_create_info2.setLayers(1);
        for (int32_t i = 0; i < m_framebuffers.size(); i++)
        {
            comp_attachments[0] = m_swapchain.view(i);
            auto res = m_context.device().createFramebuffer(&fb_create_info2, nullptr, &m_framebuffers[i]);
        }

        m_pipelines["composite_ao"] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) })
            .add_descriptor_set_layout(m_composite_render_target.descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .add_shader("passthrough.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("post.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("Composite")
            .create_graphics_pipeline(m_composite_render_target.render_pass);
        #pragma endregion

        std::string cube = "cube", sphere = "sphere";
        m_meshes[cube] = std::make_shared<sdvk::Mesh>(new primitives::Cube(), m_command_buffers, m_context, cube);
        m_meshes[sphere] = std::make_shared<sdvk::Mesh>(new primitives::Sphere(1.0f, 120), m_command_buffers, m_context, sphere);

        load_objects_from_json("objects.json");
        /*for (int32_t i = -12; i < 13; i++)
        {
            Object obj;
            obj.name = "cube" + std::to_string(i + 12);
            obj.pipeline = "default";
            obj.color = glm::vec4((float) (i + 12) / 12.0f);
            obj.transform.position = { i, 0.5, 12 };
            obj.mesh = m_meshes["cube"];
            m_objects.push_back(obj);

            if (i != -12)
            {
                Object obj2;
                obj2.name = "cube" + std::to_string(i + 12 + 25);
                obj2.pipeline = "default";
                obj2.color = glm::vec4((float) (i + 12) / 12.0f);
                obj2.transform.position = { -12, 0.5, -i };
                obj2.mesh = m_meshes["cube"];
                m_objects.push_back(obj2);
            }
        }*/

        if (m_context.raytracing())
        {
            m_tlas = sdvk::Tlas::Builder().with_name("[TLAS] Scene").create(m_objects, m_command_buffers, m_context);
            vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &m_tlas->tlas() };
            for (int32_t i = 0; i < m_swapchain.image_count(); i++)
            {
                sdvk::DescriptorWrites(m_context.device(), *m_offscreen_render_target.descriptor).acceleration_structure(i, 1, as_info).commit();
            }
        }

        if (!m_context.raytracing() && m_render_settings.ambient_occlusion.mode == AmbientOcclusionMode::eRTAO)
        {
            throw std::runtime_error("RT Ambient Occlusion required ray tracing support.");
        }

        if (m_render_settings.ambient_occlusion.mode == AmbientOcclusionMode::eRTAO)
        {
            m_ambient_occlusion = RTAOKernel::Builder()
                .with_g_buffer(m_offscreen_render_target.g_buffer)
                .with_tlas(m_tlas)
                .with_shader("dummy.comp.spv")
                .create(m_command_buffers, m_context);
        }
    }

    void Scene::rasterize(uint32_t current_frame, const vk::CommandBuffer& command_buffer)
    {
        #pragma region offscreen render pass
        update_offscreen_descriptors(current_frame);

        auto vp = m_swapchain.make_viewport();
        auto sc = m_swapchain.make_scissor();
        command_buffer.setViewport(0, 1, &vp);
        command_buffer.setScissor(0, 1, &sc);

        auto offscreen_pass_cmds = [&](const vk::CommandBuffer& cmd){
            for (const auto& obj : m_objects)
            {
                const auto& pl = m_pipelines[obj.pipeline];

                Object::PushConstantData pcd;
                pcd.model = obj.transform.model();
                pcd.color = obj.color;

                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pl.pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pl.pipeline_layout, 0, 1, &m_offscreen_render_target.descriptor->set(current_frame), 0, nullptr);
                cmd.pushConstants(pl.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData), &pcd);
                obj.mesh->draw(cmd);
            }
        };

        sdvk::RenderPass::Execute()
            .with_render_pass(m_offscreen_render_target.render_pass)
            .with_render_area({{0, 0}, m_swapchain.extent()})
            .with_clear_values(m_offscreen_render_target.clear_values)
            .with_framebuffer(m_offscreen_render_target.framebuffer)
            .execute(command_buffer, offscreen_pass_cmds);
        #pragma endregion

        #pragma region ambient occlusion
        if (m_render_settings.ambient_occlusion.is_enabled())
        {
            m_ambient_occlusion->run(m_camera->view(), command_buffer);
        }
        #pragma endregion

        #pragma region composite render pass
        update_composite_descriptors();
        auto composite_pass_cmds = [&](const vk::CommandBuffer& cmd){
            auto aspect_ratio = m_swapchain.extent();

            auto& active_pipeline = m_pipelines["composite"];
            if (m_render_settings.ambient_occlusion.is_enabled())
            {
                active_pipeline = m_pipelines["composite_ao"];
            }

            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, active_pipeline.pipeline);
            cmd.pushConstants(active_pipeline.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), &aspect_ratio);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, active_pipeline.pipeline_layout, 0, 1, &m_composite_render_target.descriptor->set(0), 0, nullptr);

            cmd.draw(3, 1, 0, 0);
        };

        vk::ImageMemoryBarrier os_barrier;
        #pragma region offscreen image barrier
        os_barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        os_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        os_barrier.setImage(m_offscreen_render_target.output->image());
        os_barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        os_barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        os_barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        #pragma endregion
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &os_barrier);

        sdvk::RenderPass::Execute()
            .with_render_pass(m_composite_render_target.render_pass)
            .with_render_area({{0, 0}, m_swapchain.extent()})
            .with_clear_values(m_offscreen_render_target.clear_values)
            .with_framebuffer(m_framebuffers[current_frame])
            .execute(command_buffer, composite_pass_cmds);

        os_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        os_barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        os_barrier.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        os_barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, 0, nullptr, 0, nullptr, 1, &os_barrier);
        #pragma endregion
    }

    void Scene::register_keybinds(GLFWwindow* p_window)
    {
        m_camera->register_keys(p_window);
    }

    void Scene::update_offscreen_descriptors(uint32_t current_frame)
    {
        auto camera_data = m_camera->uniform_data();
        m_uniform_camera[current_frame]->set_data(&camera_data, m_context.device());
    }

    void Scene::update_composite_descriptors()
    {
        vk::DescriptorImageInfo offscreen = { m_composite_render_target.sampler, m_offscreen_render_target.output->view(), vk::ImageLayout::eShaderReadOnlyOptimal };
        vk::DescriptorImageInfo ao_buffer = { m_composite_render_target.sampler, m_ambient_occlusion->get_result().view(), vk::ImageLayout::eShaderReadOnlyOptimal };

        sdvk::DescriptorWrites(m_context.device(), *m_composite_render_target.descriptor)
            .combined_image_sampler(0, 0, offscreen)
            .combined_image_sampler(0, 1, ao_buffer)
            .commit();
    }

    void Scene::load_objects_from_json(const std::string& objects_json)
    {
        using json = nlohmann::json;
        std::ifstream file(objects_json);
        auto data = json::parse(file);
        auto objects = data["objects"];

        for (const auto& obj : objects)
        {
            auto transform_props = obj["transform"];
            std::vector<float> position = transform_props["position"];
            std::vector<float> scale = transform_props["scale"];
            std::vector<float> color = obj["color"];

            auto name = obj["name"].get<std::string>();
            if (position.size() != 3)
            {
                throw std::runtime_error("[" + name + "] Position must have 3 float values.");
            }
            if (scale.size() != 3)
            {
                throw std::runtime_error("[" + name + "] Scale must have 3 float values.");
            }
            if (color.size() != 4)
            {
                throw std::runtime_error("[" + name + "] Color must have 4 float values.");
            }

            Object object;
            object.mesh = m_meshes[obj["mesh"].get<std::string>()];
            object.transform.position = glm::vec3(position[0], position[1], position[2]);
            object.transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
            object.color = glm::vec4(color[0], color[1], color[2], color[3]);
            object.pipeline = obj["pipeline"].get<std::string>();
            object.name = name;

            m_objects.push_back(object);
        }

        file.close();
    }
}