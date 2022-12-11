#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan.hpp>
#include <rt/AccelerationStructure.hpp>
#include <Utility/Math.hpp>
#include <vk/Buffer.hpp>
#include <vk/InstanceData.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Mesh.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>

struct RtAccelerator
{
    BlasInfo blas;
    TlasInfo tlas;
    std::unique_ptr<re::Buffer> instance_buffer;
    std::vector<vk::AccelerationStructureInstanceKHR> mapped_instance_buffer;

    static RtAccelerator create_accelerator(const re::InstancedGeometry& objects,
                                            const CommandBuffer& command_buffers);

    static void rebuild_top_level(re::InstancedGeometry& objects,
                                  RtAccelerator& accelerator,
                                  const CommandBuffer& command_buffers);
};
