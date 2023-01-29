#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Utils.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Resources/VertexData.hpp>

namespace sdvk
{
    class Buffer
    {
    public:
        struct Builder
        {
            Builder() = default;

            Builder& with_size(vk::DeviceSize buffer_size);
            Builder& with_usage_flags(vk::BufferUsageFlags usage_flags);
            Builder& with_memory_property_flags(vk::MemoryPropertyFlags memory_property_flags);
            Builder& with_name(std::string const& name);

            Builder& as_uniform_buffer();

            Builder& as_vertex_buffer();

            Builder& as_index_buffer();

            Builder& as_storage_buffer();

            Builder& as_acceleration_structure_storage();

            Builder& as_shader_binding_table();

            std::unique_ptr<Buffer> create(Context const& ctx);

            std::unique_ptr<Buffer> create_staging(Context const& ctx);

            template <typename T>
            std::unique_ptr<Buffer> create_with_data(T* p_data, CommandBuffers const& command_buffers, Context const& ctx)
            {
                auto result = std::make_unique<Buffer>(_buffer_size, _usage_flags, _memory_property_flags, ctx);
                if (!_name.empty())
                {
                    sdvk::util::name_vk_object(_name, (uint64_t) static_cast<VkBuffer>(result->m_buffer), vk::ObjectType::eBuffer, ctx.device());
                }

                auto staging = Buffer::Builder().with_size(_buffer_size).create_staging(ctx);
                staging->set_data(p_data, ctx.device());
                Buffer::copy_to_buffer(*staging, *result, command_buffers);

                return result;
            }

        private:
            vk::DeviceSize _buffer_size { 0 };
            vk::BufferUsageFlags _usage_flags {};
            vk::MemoryPropertyFlags _memory_property_flags {};
            std::string _name;
        };

        Buffer(Buffer const&) = delete;
        Buffer& operator=(Buffer const&) = delete;

        Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_property_flags, Context const& ctx);

        template <typename T>
        void set_data(T* p_data, vk::Device const& device)
        {
            void* mapped_memory = nullptr;
            auto result = device.mapMemory(m_memory, 0, m_size, {}, &mapped_memory);
            std::memcpy(mapped_memory, p_data, static_cast<size_t>(m_size));
            device.unmapMemory(m_memory);
        }

        const vk::Buffer& buffer() const { return m_buffer; }

        const vk::DeviceAddress& address() const { return m_address; }

        static void copy_to_buffer(Buffer const& src, Buffer const& dst, CommandBuffers const& command_buffers);

        void copy_to_buffer(Buffer const& dst, vk::CommandBuffer const& command_buffer);

        static void copy_to_image(Buffer const& src, Image const& dst, CommandBuffers const& command_buffers);

        void copy_to_image(Image const& dst, vk::CommandBuffer const& command_buffer);

    protected:
        vk::Buffer              m_buffer;
        vk::DeviceMemory        m_memory;
        vk::DeviceAddress       m_address;

        vk::DeviceSize          m_size;
        vk::BufferUsageFlags    m_usage_flags;
        vk::MemoryPropertyFlags m_mem_flags;
    };
}
