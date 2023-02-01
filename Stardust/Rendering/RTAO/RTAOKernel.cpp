#include "RTAOKernel.hpp"

#include <Vulkan/Image/Sampler.hpp>
#include <Vulkan/Descriptors/DescriptorBuilder.hpp>
#include <Vulkan/Rendering/PipelineBuilder.hpp>

namespace sd
{

    RTAOKernel::RTAOKernel(const std::shared_ptr<sdvk::Image>& g_buffer, const std::shared_ptr<sdvk::Tlas>& tlas,
                           const std::string& compute_shader, const sdvk::CommandBuffers& command_buffers,
                           const sdvk::Context& context)
    : AmbientOcclusion(command_buffers, context)
    {
        m_g_buffer = std::shared_ptr<sdvk::Image>(g_buffer);
        m_tlas = std::shared_ptr<sdvk::Tlas>(tlas);
        create_resources();
        create_pipeline(compute_shader);
    }

    void RTAOKernel::create_resources()
    {
        m_sampler = sdvk::SamplerBuilder().create(m_context.device());

        m_ao_buffer = sdvk::Image::Builder()
            .with_extent(m_g_buffer->extent())
            .with_format(vk::Format::eR32Sfloat)
            .with_usage_flags(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage)
            .with_memory_property_flags(vk::MemoryPropertyFlagBits::eDeviceLocal)
            .with_name("[RTAO] AO-Buffer")
            .create(m_context);

        m_command_buffers.execute_single_time([&](const vk::CommandBuffer& cmd){
            m_ao_buffer->transition_layout(cmd, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
        });

        m_ao_descriptor = sdvk::DescriptorBuilder()
            .storage_image(0, vk::ShaderStageFlagBits::eCompute)
            .storage_image(1, vk::ShaderStageFlagBits::eCompute)
            .accelerator(2, vk::ShaderStageFlagBits::eCompute)
            .with_name("RTAO Kernel")
            .create(m_context.device(), 1);

        m_descriptor_write_cache.tlas = { 1, &m_tlas->tlas() };
        m_descriptor_write_cache.g_buffer = { m_sampler, m_g_buffer->view(), vk::ImageLayout::eGeneral };
        m_descriptor_write_cache.ao_buffer = { m_sampler, m_ao_buffer->view(), vk::ImageLayout::eGeneral };

        update_descriptors();
    }

    void RTAOKernel::create_pipeline(const std::string& compute_shader)
    {
        m_ao_pipeline = sdvk::PipelineBuilder(m_context)
            .add_push_constant({ vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAOParams) })
            .add_descriptor_set_layout(m_ao_descriptor->layout())
            .create_pipeline_layout()
            .add_shader(compute_shader, vk::ShaderStageFlagBits::eCompute)
            .with_name("RTAO Compute")
            .create_compute_pipeline();
    }

    void RTAOKernel::update_descriptors()
    {
        std::vector<vk::WriteDescriptorSet> writes;
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(2);
            write.setDstSet(m_ao_descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
            write.setDstArrayElement(0);
            write.setPNext(&m_descriptor_write_cache.tlas);
            writes.push_back(write);
        }
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(1);
            write.setDstSet(m_ao_descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(m_descriptor_write_cache.ao_buffer);
            writes.push_back(write);
        }
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(0);
            write.setDstSet(m_ao_descriptor->set(0));
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setDstArrayElement(0);
            write.setImageInfo(m_descriptor_write_cache.g_buffer);
            writes.push_back(write);
        }

        m_context.device().updateDescriptorSets(writes.size(),writes.data(), 0, nullptr);
    }

    void RTAOKernel::run(const glm::mat4& view_mtx, const vk::CommandBuffer& command_buffer)
    {
        update_descriptors();
        m_ao_params.acc_frames = m_frame;

        // AO accumulation when camera is stationary.
        static glm::mat4 ref_mat = glm::mat4(1.0f);
        if (view_mtx != ref_mat)
        {
            ref_mat = view_mtx;
            m_frame = -1;
        }
        m_frame++;

        /**
         * Expected G-Buffer properties:
         * Render : eColorAttachmentWrite, eColorAttachmentOptimal
         * For AO : eShaderRead, eGeneral
         */
        vk::ImageMemoryBarrier g_buffer_barrier;
        #pragma region barrier setup
        g_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        g_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        g_buffer_barrier.setImage(m_g_buffer->image());
        g_buffer_barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        g_buffer_barrier.setNewLayout(vk::ImageLayout::eGeneral);
        g_buffer_barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        #pragma endregion

        /**
         * Expected AO-Buffer properties:
         * Render : eShaderRead, eShaderReadOnlyOptimal
         * For AO : eShaderWrite, eGeneral
         */
        vk::ImageMemoryBarrier ao_buffer_barrier;
        #pragma region barrier setup
        ao_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
        ao_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderWrite);
        ao_buffer_barrier.setImage(m_ao_buffer->image());
        ao_buffer_barrier.setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        ao_buffer_barrier.setNewLayout(vk::ImageLayout::eGeneral);
        ao_buffer_barrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        #pragma endregion
        static bool first = true;
        if (!first)
        {
            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eComputeShader, {}, 0, nullptr, 0, nullptr, 1, &ao_buffer_barrier);
        }
        first = false;

        auto size = m_ao_buffer->extent();
        auto group_x = (size.width + (s_group_size - 1)) / s_group_size;
        auto group_y = (size.height + (s_group_size - 1)) / s_group_size;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eDeviceGroup, 0, nullptr, 0, nullptr, 1, &g_buffer_barrier);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_ao_pipeline.pipeline);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_ao_pipeline.pipeline_layout, 0, 1, &m_ao_descriptor->set(0), 0, nullptr);
        command_buffer.pushConstants(m_ao_pipeline.pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(RTAOParams), &m_ao_params);
        command_buffer.dispatch(group_x, group_y, 1);

        ao_buffer_barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
        ao_buffer_barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        ao_buffer_barrier.setOldLayout(vk::ImageLayout::eGeneral);
        ao_buffer_barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader ,{}, 0, nullptr, 0, nullptr, 1, &ao_buffer_barrier);
    }
}