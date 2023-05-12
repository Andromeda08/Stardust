#include "RTAONode.hpp"

#include <glm/ext/matrix_relational.hpp>
#include <Application/Application.hpp>
#include <Vulkan/Barrier.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>
#include <RenderGraph/res/ImageResource.hpp>
#include <RenderGraph/res/AccelerationStructureResource.hpp>
#include <RenderGraph/res/CameraResource.hpp>
#include <RenderGraph/res/ObjectsResource.hpp>

namespace sd::rg
{
    RTAONode::RTAONode(const sdvk::Context& context, const sdvk::CommandBuffers& command_buffers)
    : Node("RTAO", {254, 100, 11, 255}, {239, 159, 118, 255})
    , m_context(context)
    {
        m_parameters.resolution = sd::Application::s_extent.vk_ext();

        _init_inputs();
        _init_outputs(command_buffers);
    }

    void RTAONode::execute(const vk::CommandBuffer& command_buffer)
    {
        // flag used for initial image layout transitions
        static bool first = true;

        _update_descriptors();

        auto camera = *dynamic_cast<CameraResource&>(*m_inputs[1]).m_resource;

        // AO accumulation while camera is stationary.
        static glm::mat4 ref_mat = glm::mat4(1.0f);
        auto view_mat = camera.view();
        auto eq = glm::equal(glm::mat4(ref_mat), view_mat, 0.001f);
        if (!(eq.x && eq.y && eq.z && eq.w))
        {
            ref_mat = view_mat;
            m_kernel.frame = -1;
        }
        m_kernel.frame++;

        m_parameters.cur_samples = m_kernel.frame;

        if (m_kernel.frame * m_parameters.cur_samples > m_parameters.max_samples)
        {
            return;
        }

        auto& g_buffer = *(dynamic_cast<ImageResource&>(*m_inputs[0])).m_resource;
        auto& ao_buffer = resource();

        // Calculate grou sizes from AO buffer extent.
        auto size = ao_buffer.extent();
        auto group_x = (size.width + (Kernel::s_group_size - 1)) / Kernel::s_group_size;
        auto group_y = (size.height + (Kernel::s_group_size - 1)) / Kernel::s_group_size;

        RTAOParams params {
            .ao_radius = m_parameters.ao_radius,
            .ao_samples = m_parameters.ao_samples,
            .ao_power = m_parameters.ao_power,
            .cause_shader = 0,
            .max_samples = m_parameters.max_samples,
            .acc_frames = m_parameters.cur_samples,
        };

        vk::ImageMemoryBarrier g_buffer_barrier;
        #pragma region barrier setup
        g_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        g_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        g_buffer_barrier.setImage(g_buffer.image());
        g_buffer_barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        g_buffer_barrier.setNewLayout(vk::ImageLayout::eGeneral);
        g_buffer_barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        #pragma endregion

        vk::ImageMemoryBarrier ao_buffer_barrier;
        #pragma region barrier setup
        ao_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        ao_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);
        ao_buffer_barrier.setImage(ao_buffer.image());
        ao_buffer_barrier.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        ao_buffer_barrier.setNewLayout(vk::ImageLayout::eGeneral);
        ao_buffer_barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        #pragma endregion

        if (!first)
        {
            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &ao_buffer_barrier);
        }
        first = false;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, 0, nullptr, 0, nullptr, 1, &g_buffer_barrier);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_kernel.pipeline.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_kernel.pipeline.pipeline_layout, 0, 1, &m_kernel.descriptor->set(0), 0, nullptr);
        command_buffer.pushConstants(m_kernel.pipeline.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAOParams), &params);
        command_buffer.dispatch(group_x, group_y, 1);

        ao_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
        ao_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        ao_buffer_barrier.setOldLayout(vk::ImageLayout::eGeneral);
        ao_buffer_barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader ,{}, 0, nullptr, 0, nullptr, 1, &ao_buffer_barrier);
    }

    void RTAONode::compile()
    {
        _init_kernel();
    }

    void RTAONode::draw()
    {
        ImNodes::PushColorStyle(ImNodesCol_TitleBar, get_color().operator ImU32());
        ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, get_hover_color().operator ImU32());
        ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, get_hover_color().operator ImU32());

        ImNodes::BeginNode(m_id);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("RTAO");
        ImNodes::EndNodeTitleBar();

        for (const auto& i : m_inputs)
        {
            ImNodes::PushColorStyle(ImNodesCol_Pin, i->imu32());
            ImNodes::BeginInputAttribute(i->id());
            ImGui::Text(i->get_name().c_str());
            #ifdef SD_DEBUG
                ImGui::Text(std::to_string(i->id()).c_str());
            #endif
            ImNodes::EndInputAttribute();
            ImNodes::PopColorStyle();
        }

        for (const auto& i : m_outputs)
        {
            ImNodes::PushColorStyle(ImNodesCol_Pin, i->imu32());
            ImNodes::BeginOutputAttribute(i->id());
            ImGui::Text(i->get_name().c_str());
            #ifdef SD_DEBUG
                ImGui::Text(std::to_string(i->id()).c_str());
            #endif
            ImNodes::EndOutputAttribute();
            ImNodes::PopColorStyle();
        }
        ImNodes::EndNode();

        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
        ImNodes::PopColorStyle();
    }

    void RTAONode::_init_inputs()
    {
        m_inputs.resize(3);

        // G-Buffer
        m_inputs[0] = ImageResource::Builder()
                .with_name("G-Buffer")
                .accept_formats({ vk::Format::eR32G32B32A32Sfloat })
                .create();

        // Camera
        m_inputs[1] = std::make_unique<CameraResource>("Camera");

        // TLAS
        m_inputs[2] = std::make_unique<AccelerationStructureResource>("TLAS");
    }

    void RTAONode::_init_outputs(const sdvk::CommandBuffers& command_buffers)
    {
        m_outputs.resize(1);
        auto& ao_buffer = m_outputs[0];

        std::shared_ptr<sdvk::Image> image = sdvk::Image::Builder()
                .with_extent(m_parameters.resolution)
                .with_format(vk::Format::eR32Sfloat)
                .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
                .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
                .with_name("AO Buffer")
                .create(m_context);

        command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            image->transition_layout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
        });

        ao_buffer = ImageResource::Builder().with_name("AO Buffer").create_from_resource(image);
    }

    void RTAONode::_init_kernel()
    {
        auto& k = m_kernel;

        k.sampler = sdvk::SamplerBuilder().create(m_context.device());
        k.descriptor = sdvk::DescriptorBuilder()
                .storage_image(0, vk::ShaderStageFlagBits::eCompute)
                .storage_image(1, vk::ShaderStageFlagBits::eCompute)
                .accelerator(2, vk::ShaderStageFlagBits::eCompute)
                .with_name("RTAO Node")
                .create(m_context.device(), 1);

        k.pipeline = sdvk::PipelineBuilder(m_context)
                .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAOParams) })
                .add_descriptor_set_layout(k.descriptor->layout())
                .create_pipeline_layout()
                .add_shader("rtao.comp.spv", vk::ShaderStageFlagBits::eCompute)
                .with_name("RTAO Node")
                .create_compute_pipeline();

        _update_descriptors();
    }

    void RTAONode::_update_descriptors()
    {
        auto& k = m_kernel;
        std::vector<vk::WriteDescriptorSet> writes;
        {
            auto& tl = dynamic_cast<AccelerationStructureResource&>(*m_inputs[2]);
            vk::WriteDescriptorSetAccelerationStructureKHR tl_info { 1, &tl.m_resource->tlas() };

            vk::WriteDescriptorSet write;
            write.setDstBinding(2);
            write.setDstSet(k.descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
            write.setDstArrayElement(0);
            write.setPNext(&tl_info);
            writes.push_back(write);
        }
        {
            vk::DescriptorImageInfo ao_info = { k.sampler, resource().view(), vk::ImageLayout::eGeneral };

            vk::WriteDescriptorSet write;
            write.setDstBinding(1);
            write.setDstSet(k.descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(ao_info);
            writes.push_back(write);
        }
        {
            auto& gb = dynamic_cast<ImageResource&>(*m_inputs[0]);
            vk::DescriptorImageInfo gb_info = { k.sampler, gb.m_resource->view(), vk::ImageLayout::eGeneral };

            vk::WriteDescriptorSet write;
            write.setDstBinding(0);
            write.setDstSet(k.descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(gb_info);
            writes.push_back(write);
        }

        m_context.device().updateDescriptorSets(writes.size(),writes.data(), 0, nullptr);
    }

    sdvk::Image& RTAONode::resource()
    {
        auto res = dynamic_cast<ImageResource&>(*m_outputs[0]);
        return *res.m_resource;
    }
}
