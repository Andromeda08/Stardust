#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct BlasInput
{
    std::vector<vk::AccelerationStructureGeometryKHR>       as_geometry;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> as_range_info;
    vk::BuildAccelerationStructureFlagsKHR                  as_flags {0};
};