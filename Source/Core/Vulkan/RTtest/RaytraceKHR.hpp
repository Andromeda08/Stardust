#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "../Device.hpp"
#include "../Buffer/Buffer.hpp"

class RaytracingBuilder
{
public:
    void setup(const Device& device, const CommandBuffers& cmd_buffers, uint32_t queueIndex);

    vk::AccelerationStructureKHR& acceleration_structure() const;

    vk::DeviceAddress blas_device_address(uint32_t blas_id);

    void build_blas(const std::vector<BlasInput>& input,
                    vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    void update_blas(uint32_t blad_id, BlasInput& blas, vk::BuildAccelerationStructureFlagsKHR flags);

    void build_tlas(const std::vector<vk::AccelerationStructureInstanceKHR>& instances,
                    vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    void command_create_tlas(vk::CommandBuffer cmd_buffer,
                             uint32_t instance_count,
                             vk::DeviceAddress instance_buffer_addr,
                             Buffer& scratch_buffer);

private:
    std::vector<AccelKHR> m_blas;
    AccelKHR m_tlas;

    const Device& m_device;
    const CommandBuffers* m_cmd_buffers;
    uint32_t m_queue_index;
};