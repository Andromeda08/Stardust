#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../Buffer/Buffer.hpp"
#include "../Command/CommandBuffers.hpp"

struct RtMesh;

struct BlasInfo
{
    vk::AccelerationStructureKHR blas {VK_NULL_HANDLE};
    std::unique_ptr<Buffer>      buffer;
    vk::DeviceAddress            blas_address {0};

    static BlasInfo create_blas(const RtMesh& mesh, const CommandBuffers& command_buffers, vk::DispatchLoaderDynamic dispatch);
};

struct TlasInfo
{
    vk::AccelerationStructureKHR tlas {VK_NULL_HANDLE};
    std::unique_ptr<Buffer>      buffer;
    std::unique_ptr<Buffer>      scratch_buffer;

    static TlasInfo create_tlas(uint32_t instance_count,
                                vk::DeviceAddress instance_address,
                                vk::DispatchLoaderDynamic dispatch,
                                const Device& device,
                                const CommandBuffers& command_buffers);
};