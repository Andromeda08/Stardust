#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Vulkan/Device.hpp>
#include <vk/Buffer.hpp>
#include <vk/Mesh.hpp>

struct BlasInfo
{
    vk::AccelerationStructureKHR blas {VK_NULL_HANDLE};
    std::unique_ptr<re::Buffer>  buffer;
    vk::DeviceAddress            blas_address {0};

    static BlasInfo create_blas(const re::Mesh& mesh, const CommandBuffer& command_buffers);
};

struct TlasInfo
{
    vk::AccelerationStructureKHR tlas {VK_NULL_HANDLE};
    std::unique_ptr<re::Buffer>  buffer;
    std::unique_ptr<re::Buffer>  scratch_buffer;

    static TlasInfo create_tlas(uint32_t instance_count,
                                vk::DeviceAddress instance_address,
                                const CommandBuffer& command_buffers);
};

