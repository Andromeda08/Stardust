#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Rendering/Mesh.hpp>

namespace sdvk
{
    struct Blas
    {
        vk::AccelerationStructureKHR blas;
        vk::DeviceAddress address;

        std::unique_ptr<Buffer> buffer;

        static Blas create_blas(Mesh const& mesh, CommandBuffers const& command_buffers, Context const& context);
    };
}