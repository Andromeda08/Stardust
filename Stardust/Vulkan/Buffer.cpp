#include "Buffer.hpp"

namespace sdvk
{

    Buffer::Builder& Buffer::Builder::with_size(vk::DeviceSize buffer_size)
    {
        _buffer_size = buffer_size;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::with_usage_flags(vk::BufferUsageFlags usage_flags)
    {
        _usage_flags = usage_flags;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::with_memory_property_flags(vk::MemoryPropertyFlags memory_property_flags)
    {
        _memory_property_flags = memory_property_flags;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::with_name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    std::unique_ptr<Buffer> Buffer::Builder::create(const Context& ctx)
    {
        auto result = std::make_unique<Buffer>(_buffer_size, _usage_flags, _memory_property_flags, ctx);
        if (!_name.empty())
        {
            sdvk::util::name_vk_object(_name, (uint64_t) static_cast<VkBuffer>(result->m_buffer), vk::ObjectType::eBuffer, ctx.device());
        }
        return result;
    }

    std::unique_ptr<Buffer> Buffer::Builder::create_staging(const Context& ctx)
    {
        return std::make_unique<Buffer>(_buffer_size,
                                        vk::BufferUsageFlagBits::eTransferSrc,
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                        ctx);
    }

    Buffer::Builder& Buffer::Builder::as_uniform_buffer()
    {
        _usage_flags = vk::BufferUsageFlagBits::eUniformBuffer;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::as_vertex_buffer()
    {
        _usage_flags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::as_index_buffer()
    {
        _usage_flags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::as_storage_buffer()
    {
        _usage_flags = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::as_shader_binding_table()
    {
        _usage_flags = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddressKHR;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::as_acceleration_structure_storage()
    {
        _usage_flags = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        _memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        return *this;
    }

    Buffer::Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage_flags,
                   vk::MemoryPropertyFlags memory_property_flags, const Context& ctx)
    : m_size(buffer_size), m_usage_flags(usage_flags), m_mem_flags(memory_property_flags)
    {
        vk::Result result;

        vk::BufferCreateInfo create_info;
        create_info.setSharingMode(vk::SharingMode::eExclusive);
        create_info.setSize(buffer_size);
        create_info.setUsage(usage_flags | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress);

        result = ctx.device().createBuffer(&create_info, nullptr, &m_buffer);
        auto memory_requirements = ctx.device().getBufferMemoryRequirements(m_buffer);
        ctx.allocate_memory(memory_requirements, memory_property_flags, &m_memory);

        ctx.device().bindBufferMemory(m_buffer, m_memory, 0 );

        vk::BufferDeviceAddressInfo address_info;
        address_info.setBuffer(m_buffer);
        m_address = ctx.device().getBufferAddress(&address_info);
    }

    void Buffer::copy_to_buffer(const Buffer& src, const Buffer& dst, const CommandBuffers& command_buffers)
    {
        command_buffers.execute_single_time([&src, &dst](vk::CommandBuffer const& cmd){
            vk::BufferCopy copy_region;
            copy_region.setSize(src.m_size);
            copy_region.setSrcOffset(0);
            copy_region.setDstOffset(0);

            cmd.copyBuffer(src.m_buffer, dst.m_buffer, 1, &copy_region);
        });
    }
    void Buffer::copy_to_buffer(const Buffer& dst, const vk::CommandBuffer& command_buffer)
    {
        vk::BufferCopy copy_region;
        copy_region.setSize(m_size);
        copy_region.setSrcOffset(0);
        copy_region.setDstOffset(0);
        command_buffer.copyBuffer(m_buffer, dst.m_buffer, 1, &copy_region);
    }

    void Buffer::copy_to_image(const Buffer& src, const Nebula::Image& dst, const CommandBuffers& command_buffers)
    {
        command_buffers.execute_single_time([&src, &dst](vk::CommandBuffer const& cmd){
            auto extent = dst.properties().extent;
            vk::BufferImageCopy copy_region;
            copy_region.setBufferOffset(0);
            copy_region.setBufferRowLength(0);
            copy_region.setBufferImageHeight(0);
            copy_region.setImageSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
            copy_region.setImageOffset({ 0, 0, 0 });
            copy_region.setImageExtent({ extent.width, extent.height, 1 });

            cmd.copyBufferToImage(src.m_buffer, dst.image(), vk::ImageLayout::eTransferDstOptimal, 1, &copy_region);
        });
    }

    void Buffer::copy_to_image(const Nebula::Image& dst, const vk::CommandBuffer& command_buffer)
    {
        auto extent = dst.properties().extent;
        vk::BufferImageCopy copy_region;
        copy_region.setBufferOffset(0);
        copy_region.setBufferRowLength(0);
        copy_region.setBufferImageHeight(0);
        copy_region.setImageSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
        copy_region.setImageOffset({ 0, 0, 0 });
        copy_region.setImageExtent({ extent.width, extent.height, 1 });
        command_buffer.copyBufferToImage(m_buffer, dst.image(), vk::ImageLayout::eTransferDstOptimal, 1, &copy_region);
    }
}