#include "Scene.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Descriptors/DescriptorWrites.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Image/Sampler.hpp>

namespace sd
{
    Scene::Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context, const sdvk::Swapchain& swapchain)
    : m_command_buffers(command_buffers), m_context(context), m_swapchain(swapchain)
    {
        {
            auto scext = swapchain.extent();
            m_camera = std::make_unique<Camera>(glm::ivec2(scext.width, scext.height), glm::vec3(5.0f));
        }

        m_descriptor = sdvk::DescriptorBuilder()
            .uniform_buffer(0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .accelerator(1, vk::ShaderStageFlagBits::eFragment)
            .create(context.device(), m_swapchain.image_count());

        auto cud = m_camera->uniform_data();
        m_uniform_camera.resize(m_swapchain.image_count());
        for (int32_t i = 0; i < m_swapchain.image_count(); i++)
        {
            m_uniform_camera[i] = sdvk::Buffer::Builder()
                .with_size(sizeof(CameraUniformData))
                .as_uniform_buffer()
                .create(context);

            m_uniform_camera[i]->set_data(&cud, context.device());

            vk::DescriptorBufferInfo info = { m_uniform_camera[i]->buffer(), 0, sizeof(CameraUniformData) };
            sdvk::DescriptorWrites(context.device(), *m_descriptor)
                .uniform_buffer(i, 0, info)
                .commit();
        }

        m_rendering.depth_buffer = std::make_unique<sdvk::DepthBuffer>(m_swapchain.extent(), vk::SampleCountFlagBits::e8, m_context);
        m_rendering.clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        m_rendering.clear_values[1].setDepthStencil({ 1.0f, 0 });
        m_rendering.multisampling_buffer = sdvk::Image::Builder()
            .with_format(m_swapchain.format())
            .with_extent(m_swapchain.extent())
            .with_tiling(vk::ImageTiling::eOptimal)
            .with_usage_flags(vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_sample_count(vk::SampleCountFlagBits::e8)
            .create(m_context);
        m_rendering.msaa = sdvk::RenderPass::Builder()
            .add_color_attachment(m_swapchain.format(), vk::SampleCountFlagBits::e8, vk::ImageLayout::eColorAttachmentOptimal)
            .set_depth_attachment(m_rendering.depth_buffer->format(), vk::SampleCountFlagBits::e8)
            .set_resolve_attachment(m_rendering.multisampling_buffer->format(), vk::ImageLayout::ePresentSrcKHR)
            .make_subpass()
            .with_name("MSAA RenderPass")
            .create(m_context);

        std::array<vk::ImageView, 3> attachments = { m_rendering.multisampling_buffer->view(), m_rendering.depth_buffer->view(), m_swapchain.view(0) };
        vk::FramebufferCreateInfo create_info;
        create_info.setRenderPass(m_rendering.msaa);
        create_info.setAttachmentCount(attachments.size());
        create_info.setPAttachments(attachments.data());
        create_info.setWidth(m_swapchain.extent().width);
        create_info.setHeight(m_swapchain.extent().height);
        create_info.setLayers(1);

        for (size_t i = 0; i < 2; i++)
        {
            attachments[2] = m_swapchain.view(i);
            auto result = m_context.device().createFramebuffer(&create_info, nullptr, &m_rendering.framebuffers[i]);
        }

        m_pipelines["default"] = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData) })
            .add_descriptor_set_layout(m_descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e8)
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader("rq_light.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rq_light.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("MSAA Graphics")
            .create_graphics_pipeline(m_rendering.msaa);

        std::string cube = "cube", sphere = "sphere";
        m_meshes[cube] = std::make_shared<sdvk::Mesh>(new primitives::Cube(), m_command_buffers, m_context, cube);
        m_meshes[sphere] = std::make_shared<sdvk::Mesh>(new primitives::Sphere(1.0f, 120), m_command_buffers, m_context, sphere);

        load_objects_from_json("objects.json");
        for (int32_t i = -12; i < 13; i++)
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
        }

        if (m_context.raytracing())
        {
            m_tlas = sdvk::Tlas::Builder().with_name("[TLAS] Scene").create(m_objects, m_command_buffers, m_context);
            vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &m_tlas->tlas() };
            for (int32_t i = 0; i < m_swapchain.image_count(); i++)
            {
                sdvk::DescriptorWrites(m_context.device(), *m_descriptor).acceleration_structure(i, 1, as_info).commit();
            }
        }

        init_rtao();
    }

    void Scene::rasterize(uint32_t current_frame, const vk::CommandBuffer& cmd)
    {
        auto cud = m_camera->uniform_data();
        m_uniform_camera[current_frame]->set_data(&cud, m_context.device());

        auto viewport = m_swapchain.make_viewport();
        auto scissor = m_swapchain.make_scissor();
        cmd.setViewport(0, 1, &viewport);
        cmd.setScissor(0, 1, &scissor);

        auto commands = [&](const vk::CommandBuffer& cmd){
            for (const auto& obj : m_objects)
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipelines[obj.pipeline].pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelines[obj.pipeline].pipeline_layout, 0, 1, &m_descriptor->set(current_frame), 0, nullptr);

                Object::PushConstantData obj_pcd;
                obj_pcd.model = obj.transform.model();
                obj_pcd.color = obj.color;

                cmd.pushConstants(m_pipelines[obj.pipeline].pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData), &obj_pcd);

                obj.mesh->draw(cmd);
            }
        };

        sdvk::RenderPass::Execute()
            .with_render_pass(m_rendering.msaa)
            .with_render_area({{0, 0}, m_swapchain.extent()})
            .with_clear_values(m_rendering.clear_values)
            .with_framebuffer(m_rendering.framebuffers[current_frame])
            .execute(cmd, commands);

        auto commands2 = [&](const vk::CommandBuffer& cmd){
            for (const auto& obj : m_objects)
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_rtao.pipeline.pipeline);
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_rtao.pipeline.pipeline_layout, 0, 1, &m_descriptor->set(current_frame), 0, nullptr);

                Object::PushConstantData obj_pcd;
                obj_pcd.model = obj.transform.model();
                obj_pcd.color = obj.color;

                cmd.pushConstants(m_rtao.pipeline.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData), &obj_pcd);

                obj.mesh->draw(cmd);
            }
        };

        sdvk::RenderPass::Execute()
            .with_render_pass(m_rtao.render_pass)
            .with_render_area({{0, 0}, m_swapchain.extent()})
            .with_clear_values(m_rtao.clear_values)
            .with_framebuffer(m_rtao.framebuffer)
            .execute(cmd, commands2);

        run_compute(cmd);

        auto commands3 = [&](const vk::CommandBuffer& cmd){
            std::vector<vk::WriteDescriptorSet> _writes;
            vk::DescriptorImageInfo offscreen = {}, aoresult = {};
            offscreen.setSampler(m_rtao.sampler);
            offscreen.setImageView(m_rtao.offscreen->view());
            offscreen.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            aoresult.setSampler(m_rtao.sampler);
            aoresult.setImageView(m_rtao.aobuffer->view());
            aoresult.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
            {
                vk::WriteDescriptorSet write;
                write.setDstBinding(1);
                write.setDstSet(m_post.post_desc->set(0));
                write.setDescriptorCount(1);
                write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
                write.setDstArrayElement(0);
                write.setImageInfo(aoresult);
                _writes.push_back(write);
            }
            {
                vk::WriteDescriptorSet write;
                write.setDstBinding(0);
                write.setDstSet(m_post.post_desc->set(0));
                write.setDescriptorCount(1);
                write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
                write.setDstArrayElement(0);
                write.setImageInfo(offscreen);
                _writes.push_back(write);
            }
            m_context.device().updateDescriptorSets(_writes.size(),_writes.data(), 0, nullptr);

            auto ar = m_swapchain.aspect_ratio();
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_post.post.pipeline);
            cmd.pushConstants(m_post.post.pipeline_layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), &ar);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_post.post.pipeline_layout, 0, 1, &m_post.post_desc->set(0), 0, nullptr);
            cmd.draw(3, 1, 0, 0);
        };

        vk::ImageMemoryBarrier barrier;
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier.setImage(m_rtao.offscreen->image());
        barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);

        barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier.setImage(m_rtao.aobuffer->image());
        barrier.setOldLayout(vk::ImageLayout::eGeneral);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier);

        sdvk::RenderPass::Execute()
            .with_render_pass(m_post.render_pass)
            .with_render_area({{0, 0}, m_swapchain.extent()})
            .with_clear_values(m_rendering.clear_values)
            .with_framebuffer(m_post.framebuffer)
            .execute(cmd, commands3);

        barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setImage(m_rtao.offscreen->image());
        barrier.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, 0, nullptr, 0, nullptr, 1, &barrier);

        barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);
        barrier.setImage(m_rtao.aobuffer->image());
        barrier.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setNewLayout(vk::ImageLayout::eGeneral);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eComputeShader,{}, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void Scene::register_keybinds(GLFWwindow* p_window)
    {
        m_camera->register_keys(p_window);
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

    void Scene::init_rtao()
    {
        m_rtao.sampler = sdvk::SamplerBuilder().create(m_context.device());

        #pragma region images & depth buffer
        m_rtao.gbuffer = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(vk::Format::eR32G32B32A32Sfloat)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("RTAO G-Buffer")
            .create(m_context);

        m_rtao.aobuffer = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(vk::Format::eR32Sfloat)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("RTAO AO-Buffer")
            .create(m_context);

        m_rtao.offscreen = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(m_swapchain.format())
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("RTAO Offscreen Render")
            .create(m_context);

        m_rtao.offscreen_depth = std::make_unique<sdvk::DepthBuffer>(m_swapchain.extent(), vk::SampleCountFlagBits::e1, m_context);
        #pragma endregion

        #pragma region image layout transitions
        m_command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            m_rtao.aobuffer->transition_layout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
        });
        #pragma endregion

        m_rtao.clear_values[0].setColor(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 0.1f });
        m_rtao.clear_values[1].setColor(std::array<float, 4>{ 0, 0, 0, 0 });
        m_rtao.clear_values[2].setDepthStencil({ 1.0f, 0 });

        m_rtao.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_rtao.offscreen->format())
            .add_color_attachment(m_rtao.gbuffer->format())
            .set_depth_attachment(m_rtao.offscreen_depth->format())
            .make_subpass()
            .with_name("RTAO Offscreen RenderPass")
            .create(m_context);

        #pragma region framebuffer creation
        std::vector<vk::ImageView> attachments = { m_rtao.offscreen->view(), m_rtao.gbuffer->view(), m_rtao.offscreen_depth->view() };
        vk::FramebufferCreateInfo create_info;
        create_info.setRenderPass(m_rtao.render_pass);
        create_info.setAttachmentCount(attachments.size());
        create_info.setPAttachments(attachments.data());
        create_info.setWidth(m_swapchain.extent().width);
        create_info.setHeight(m_swapchain.extent().height);
        create_info.setLayers(1);
        auto result = m_context.device().createFramebuffer(&create_info, nullptr, &m_rtao.framebuffer);
        #pragma endregion

        m_rtao.pipeline = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eVertex, 0, sizeof(Object::PushConstantData) })
            .add_descriptor_set_layout(m_descriptor->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .set_attachment_count(2)
            .add_attribute_descriptions({ VertexData::attribute_descriptions() })
            .add_binding_descriptions({ VertexData::binding_description() })
            .add_shader("rq_light.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("rtao.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .with_name("RTAO Offscreen Graphics")
            .create_graphics_pipeline(m_rtao.render_pass);

        #pragma region compute
        m_rtao.comp_desc = sdvk::DescriptorBuilder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .accelerator(2, vk::ShaderStageFlagBits::eCompute)
            .create(m_context.device(), 1);

        vk::WriteDescriptorSetAccelerationStructureKHR as_info { 1, &m_tlas->tlas() };
        vk::DescriptorImageInfo gbuffer = {}, aobuffer = {};
        gbuffer.setSampler(m_rtao.sampler);
        gbuffer.setImageView(m_rtao.gbuffer->view());
        gbuffer.setImageLayout(vk::ImageLayout::eGeneral);
        aobuffer.setSampler(m_rtao.sampler);
        aobuffer.setImageView(m_rtao.aobuffer->view());
        aobuffer.setImageLayout(vk::ImageLayout::eGeneral);

        sdvk::DescriptorWrites(m_context.device(), *m_rtao.comp_desc)
            .storage_image(0, 0, gbuffer)
            .storage_image(0, 1, aobuffer)
            .acceleration_structure(0, 2, as_info)
            .commit();

        m_rtao.compute = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAO::AoParams) })
            .add_descriptor_set_layout(m_rtao.comp_desc->layout())
            .create_pipeline_layout()
            .add_shader("dummy.comp.spv", vk::ShaderStageFlagBits::eCompute)
            .with_name("RTAO Compute")
            .create_compute_pipeline();
        #pragma endregion

        #pragma region post
        m_post.composite = sdvk::Image::Builder()
            .with_extent(m_swapchain.extent())
            .with_format(vk::Format::eR32G32B32A32Sfloat)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("Post Composite")
            .create(m_context);

        m_post.post_desc = sdvk::DescriptorBuilder()
            .combined_image_sampler(0, vk::ShaderStageFlagBits::eFragment)
            .combined_image_sampler(1, vk::ShaderStageFlagBits::eFragment)
            .create(m_context.device(), 1);

        m_post.render_pass = sdvk::RenderPass::Builder()
            .add_color_attachment(m_post.composite->format())
            .make_subpass()
            .with_name("RTAO Post RenderPass")
            .create(m_context);

        std::vector<vk::ImageView> attachments2 = { m_post.composite->view() };
        vk::FramebufferCreateInfo ci;
        ci.setRenderPass(m_post.render_pass);
        ci.setAttachmentCount(attachments2.size());
        ci.setPAttachments(attachments2.data());
        ci.setWidth(m_swapchain.extent().width);
        ci.setHeight(m_swapchain.extent().height);
        ci.setLayers(1);
        result = m_context.device().createFramebuffer(&ci, nullptr, &m_post.framebuffer);

        m_post.post = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eFragment, 0, sizeof(float) })
            .add_descriptor_set_layout(m_post.post_desc->layout())
            .create_pipeline_layout()
            .set_sample_count(vk::SampleCountFlagBits::e1)
            .add_shader("passthrough.vert.spv", vk::ShaderStageFlagBits::eVertex)
            .add_shader("post.frag.spv", vk::ShaderStageFlagBits::eFragment)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .with_name("RTAO Post")
            .create_graphics_pipeline(m_post.render_pass);

        #pragma endregion
    }

    #define GROUP_SIZE 16
    void Scene::run_compute(const vk::CommandBuffer& cmd)
    {
        vk::DescriptorImageInfo gbuffer = {}, aobuffer = {};
        gbuffer.setSampler(m_rtao.sampler);
        gbuffer.setImageView(m_rtao.gbuffer->view());
        gbuffer.setImageLayout(vk::ImageLayout::eGeneral);
        aobuffer.setSampler(m_rtao.sampler);
        aobuffer.setImageView(m_rtao.aobuffer->view());
        aobuffer.setImageLayout(vk::ImageLayout::eGeneral);

        std::vector<vk::WriteDescriptorSet> _writes;
        {
            vk::WriteDescriptorSet write;
            vk::WriteDescriptorSetAccelerationStructureKHR as_info{1, &m_tlas->tlas()};
            write.setDstBinding(2);
            write.setDstSet(m_rtao.comp_desc->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
            write.setDstArrayElement(0);
            write.setPNext(&as_info);
            _writes.push_back(write);
        }
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(1);
            write.setDstSet(m_rtao.comp_desc->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(aobuffer);
            _writes.push_back(write);
        }
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(0);
            write.setDstSet(m_rtao.comp_desc->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(gbuffer);
            _writes.push_back(write);
        }

        m_context.device().updateDescriptorSets(_writes.size(),_writes.data(), 0, nullptr);

        static glm::mat4 ref_mat = glm::mat4(1.0f);
        auto m = m_camera->view();
        if (m != ref_mat)
        {
            ref_mat = m;
            m_rtao.frame = -1;
        }
        m_rtao.frame++;

        vk::ImageMemoryBarrier barrier;
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        barrier.setImage(m_rtao.gbuffer->image());
        barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        barrier.setNewLayout(vk::ImageLayout::eGeneral);
        barrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, 0, nullptr, 0, nullptr, 1, &barrier);
        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_rtao.compute.pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_rtao.compute.pipeline_layout, 0, 1, &m_rtao.comp_desc->set(0), 0, nullptr);
        m_rtao.params.frame = m_rtao.frame;
        cmd.pushConstants(m_rtao.compute.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAO::AoParams), &m_rtao.params);
        auto size = m_swapchain.extent();
        cmd.dispatch((size.width + (GROUP_SIZE - 1)) / GROUP_SIZE, (size.height + (GROUP_SIZE - 1)) / GROUP_SIZE, 1);

    }

}