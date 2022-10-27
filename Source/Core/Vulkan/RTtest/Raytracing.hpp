#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "AccelerationStructure.hpp"
#include "BlasInput.hpp"
#include "../Device.hpp"
#include "../Command/CommandBuffers.hpp"

class RaytracingBuilder
{
public:
    RaytracingBuilder::RaytracingBuilder(const Device& device,
                                         const CommandBuffers& cmds,
                                         uint32_t queue_index);

    void build_blas(const std::vector<BlasInput>& input,
                    vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

private:
    struct BuildAccelerationStructure
    {
        vk::AccelerationStructureBuildGeometryInfoKHR        as_geometry_info;
        vk::AccelerationStructureBuildSizesInfoKHR           as_size_info;
        const vk::AccelerationStructureBuildRangeInfoKHR*    as_range_info;
        AccelerationStructure                                as_result;
    };

    // BL & TL Acceleration Structures
    std::vector<AccelerationStructure>     m_blas;
    std::unique_ptr<AccelerationStructure> m_tlas;

    // Vulkan stuff
    Device& m_device;
    CommandBuffers& m_cmds;
    uint32_t m_queue_index { 0 };
};
