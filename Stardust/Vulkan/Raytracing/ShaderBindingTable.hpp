#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Context.hpp>

namespace sd::rt
{
    class ShaderBindingTable
    {
    public:
        // TODO: Possible builder struct with more options.
        ShaderBindingTable(uint32_t miss_count, uint32_t hit_count, vk::Pipeline& pipeline, const sdvk::Context& context)
        : m_hit_count(hit_count), m_miss_count(miss_count), m_pipeline(pipeline)
        {
            build(context);
        }

        void build(const sdvk::Context& context)
        {
            vk::PhysicalDeviceProperties2 props2;
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rt_props;
            props2.pNext = &rt_props;

            context.physical_device().getProperties2(&props2);

            auto handle_size    = rt_props.shaderGroupHandleSize;
            auto base_alignment = rt_props.shaderGroupBaseAlignment;
            auto handle_count   = 1 + m_miss_count + m_hit_count + m_call_count;

            uint32_t handle_size_aligned = align_up(handle_size, rt_props.shaderGroupHandleAlignment);

            #pragma region Group alignments
            // rgen size must equal stride
            m_rgen.setStride(align_up(handle_size_aligned, base_alignment));
            m_rgen.setSize(m_rgen.stride);

            m_miss.setStride(handle_size_aligned);
            m_miss.setSize(align_up(m_miss_count * handle_size_aligned, base_alignment));

            m_hit.setStride(handle_size_aligned);
            m_hit.setSize(align_up(m_hit_count * handle_size_aligned, base_alignment));

            m_call.setStride(handle_size_aligned);
            m_call.setStride(align_up(m_call_count * handle_size_aligned, base_alignment));
            #pragma endregion

            uint32_t data_size = handle_count * handle_size;
            std::vector<uint8_t> handles(data_size);
            {
                auto result = context.device().getRayTracingShaderGroupHandlesKHR(m_pipeline, 0, handle_count, data_size, handles.data());
            }

            vk::DeviceSize sbt_size = m_rgen.size + m_miss.size + m_hit.size; //+ m_call.size;
            m_buffer = sdvk::Buffer::Builder()
                .with_size(sbt_size)
                .as_shader_binding_table()
                .with_name("SBT")
                .create(context);

            auto sbt_address = m_buffer->address();
            m_rgen.setDeviceAddress(sbt_address);
            m_miss.setDeviceAddress(sbt_address + m_rgen.size);
            m_hit.setDeviceAddress(sbt_address + m_rgen.size + m_miss.size);
            m_call.setDeviceAddress((m_call_count == 0) ? 0 : sbt_address + m_rgen.size + m_miss.size + m_hit.size);

            auto get_handle = [&](uint32_t i) { return handles.data() + i * handle_size; };

            // Map buffer memory
            void* sbt = nullptr;
            {
                auto result = context.device().mapMemory(m_buffer->memory(), 0, sbt_size, {}, &sbt);
            }

            #pragma region Copy data
            uint8_t* p_sbt = reinterpret_cast<uint8_t*>(sbt);
            uint8_t* p_data {nullptr};
            uint32_t handle_idx {0};

            p_data = p_sbt;
            std::memcpy(p_data, get_handle(handle_idx++), handle_size);

            p_data = reinterpret_cast<uint8_t*>(sbt) + m_rgen.size;

            for (uint32_t i = 0; i < m_miss_count; i++)
            {
                std::memcpy(p_data, get_handle(handle_idx++), handle_size);
                p_data += m_miss.stride;
            }

            p_data = reinterpret_cast<uint8_t*>(sbt) + m_rgen.size + m_miss.size;
            for (uint32_t i = 0; i < m_hit_count; i++)
            {
                std::memcpy(p_data, get_handle(handle_idx++), handle_size);
                p_data += m_hit.stride;
            }
            #pragma endregion

            // Release buffer memory
            context.device().unmapMemory(m_buffer->memory());
        }

        const sdvk::Buffer& sbt() const { return *m_buffer; }

        const vk::StridedDeviceAddressRegionKHR* rgen_region() const { return &m_rgen; }
        const vk::StridedDeviceAddressRegionKHR* miss_region() const { return &m_miss; }
        const vk::StridedDeviceAddressRegionKHR* hit_region() const { return &m_hit; }
        const vk::StridedDeviceAddressRegionKHR* call_region() const { return &m_call; }

    private:
        /**
         * Round up sizes to next alignment
         * https://github.com/nvpro-samples/nvpro_core/blob/master/nvh/alignment.hpp
         */
        template <class Integral>
        constexpr Integral align_up(Integral x, size_t a) noexcept
        {
            return Integral((x + (Integral(a) - 1)) & ~Integral(a - 1));
        }

    private:
        vk::Pipeline&                 m_pipeline;
        std::unique_ptr<sdvk::Buffer> m_buffer;

        uint32_t m_miss_count {1};
        uint32_t m_hit_count  {1};
        uint32_t m_call_count {0};

        // SBT Regions
        vk::StridedDeviceAddressRegionKHR m_rgen {};
        vk::StridedDeviceAddressRegionKHR m_miss {};
        vk::StridedDeviceAddressRegionKHR m_hit {};
        vk::StridedDeviceAddressRegionKHR m_call {};
    };
}