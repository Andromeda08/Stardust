#pragma once

#include <vulkan/vulkan.hpp>
#include <vk/Buffer.hpp>
#include <vk/Mesh.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Device/Device.hpp>

struct BlasInfo
{
    vk::AccelerationStructureKHR blas {VK_NULL_HANDLE};
    std::unique_ptr<re::Buffer>  buffer;
    vk::DeviceAddress            blas_address {0};

    static BlasInfo create_blas(const re::Mesh& mesh, const CommandBuffers& command_buffers);
};

struct TlasInfo
{
    vk::AccelerationStructureKHR tlas {VK_NULL_HANDLE};
    std::unique_ptr<re::Buffer>  buffer;
    std::unique_ptr<re::Buffer>  scratch_buffer;

    static TlasInfo create_tlas(uint32_t instance_count,
                                vk::DeviceAddress instance_address,
                                const CommandBuffers& command_buffers);
};

