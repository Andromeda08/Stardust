#pragma once

#include <memory>
#include <typeinfo>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <vk/Image.hpp>

namespace re
{
    /**
     * @brief The Buffer class represents a Vulkan Buffer object and the memory associated with it.
     */
    class Buffer
    {
    public:
        NON_COPIABLE(Buffer)

        Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage_flags,
               vk::MemoryPropertyFlags property_flags, const CommandBuffer& command_buffer)
        : m_command_buffer(command_buffer)
        , m_device(command_buffer.device())
        , m_size(buffer_size)
        , m_usage_flags(usage_flags)
        , m_property_flags(property_flags)
        {
            vk::Result result {};

            result = create_buffer();
            result = allocate_memory();

            m_device.handle().bindBufferMemory(m_buffer, m_memory, 0);
        }

        ~Buffer()
        {
            if (m_buffer)
            {
                m_device.handle().destroyBuffer(m_buffer, nullptr);
                m_device.handle().freeMemory(m_memory, nullptr);
                m_buffer = VK_NULL_HANDLE;
            }
        }

        const vk::Buffer& buffer() const { return m_buffer; }

        /**
         * @brief Copies specified data to target buffer.
         */
        template <typename T>
        static void set_data(T* data, const Buffer& dst, const CommandBuffer& command_buffer)
        {
            auto device = command_buffer.device().handle();
            auto dispatch = command_buffer.device().dispatch();

            void* mapped_memory = nullptr;
            auto result = device.mapMemory(dst.m_memory, 0, dst.m_size, {}, &mapped_memory, dispatch);
            std::memcpy(mapped_memory, data, static_cast<size_t>(dst.m_size));
            device.unmapMemory(dst.m_memory);
        }

        /**
         * @brief Copy data from source to destination Buffer
         */
        static void copy_buffer(const Buffer& src, const Buffer& dst, const CommandBuffer& command_buffer)
        {
            auto cmd = command_buffer.begin_single_time();
            {
                vk::BufferCopy cregion;
                cregion.setSrcOffset(0);
                cregion.setDstOffset(0);
                cregion.setSize(src.m_size);
                cmd.copyBuffer(src.m_buffer, dst.m_buffer, 1, &cregion);
            }
            command_buffer.end_single_time(cmd);
        }

        /**
         * @brief Copy buffer contents to Image
         */
        static void copy_to_image(const Buffer& src, const vkImage& image, const CommandBuffer& command_buffers)
        {
            auto cmd = command_buffers.begin_single_time();
            {
                auto extent = image.extent();

                vk::ImageSubresourceLayers subresource;
                subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
                subresource.setMipLevel(0);
                subresource.setBaseArrayLayer(0);
                subresource.setLayerCount(1);

                vk::BufferImageCopy region;
                region.setBufferOffset(0);
                region.setBufferRowLength(0);
                region.setBufferImageHeight(0);
                region.setImageSubresource(subresource);
                region.setImageOffset({ 0, 0, 0 });
                region.setImageExtent({ extent.width, extent.height, 1 });

                cmd.copyBufferToImage(src.buffer(),
                                      image.image(),
                                      vk::ImageLayout::eTransferDstOptimal,
                                      1,
                                      &region,
                                      command_buffers.device().dispatch());
            }
            command_buffers.end_single_time(cmd);
        }

        /**
         * @brief Creates a host visible staging buffer.
         */
        static std::unique_ptr<Buffer> make_staging_buffer(vk::DeviceSize buffer_size, const CommandBuffer& command_buffer)
        {
            return std::make_unique<Buffer>(buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                            command_buffer);
        }

    private:
        /**
         * @brief Allocate memory for buffer according to memory requirements.
         */
        vk::Result allocate_memory()
        {
            vk::Result result;

            auto device = m_device.handle();
            auto dispatch = m_device.dispatch();

            auto memory_requirements = device.getBufferMemoryRequirements(m_buffer, dispatch);
            vk::MemoryAllocateInfo alloc_info;
            alloc_info.setAllocationSize(memory_requirements.size);
            alloc_info.setMemoryTypeIndex(m_device.findMemoryType(memory_requirements.memoryTypeBits, m_property_flags));

            result = device.allocateMemory(&alloc_info, nullptr, &m_memory, dispatch);

            return result;
        }

        /**
         * @brief Creates a vulkan Buffer
         * @note Sharing mode is set to exclusive by default.
         */
        vk::Result create_buffer()
        {
            vk::Result result;

            auto device = m_device.handle();
            auto dispatch = m_device.dispatch();

            vk::BufferCreateInfo create_info;
            create_info.setSharingMode(vk::SharingMode::eExclusive);
            create_info.setSize(m_size);
            create_info.setUsage(m_usage_flags);

            result = device.createBuffer(&create_info, nullptr, &m_buffer, dispatch);

            return result;
        }

    protected:
        vk::Buffer              m_buffer;
        vk::DeviceMemory        m_memory;

        vk::DeviceSize          m_size;
        vk::BufferUsageFlags    m_usage_flags;
        vk::MemoryPropertyFlags m_property_flags;

        const CommandBuffer&    m_command_buffer;
        const Device&           m_device;
    };

#pragma region Specific_Buffer_Types

    template <typename T>
    class IndexBuffer : public Buffer
    {
    public:
        NON_COPIABLE(IndexBuffer)

        IndexBuffer(const std::vector<T>& indices, const CommandBuffer& command_buffer)
        : Buffer(sizeof(T) * indices.size(),
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 command_buffer)
        , m_indices(indices)
        {
            // Index type should be compatible with Vulkan
            static_assert(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint16_t>, "T must be uint32_t or uint16_t!");

            // Store Vulkan index type
            if (std::is_same_v<T, uint16_t>) m_index_type = vk::IndexType::eUint16;
            if (std::is_same_v<T, uint32_t>) m_index_type = vk::IndexType::eUint32;

            auto staging_buffer = Buffer::make_staging_buffer(m_size, command_buffer);
            Buffer::set_data(indices.data(), *staging_buffer, command_buffer);
            Buffer::copy_buffer(*staging_buffer, *this, command_buffer);
        }

        T count() const { return m_indices.size(); }

        vk::IndexType type() const { return m_index_type; }

    private:
        const std::vector<T>& m_indices;
        vk::IndexType         m_index_type;
    };

    /**
     * @brief Buffer for VertexBuffer usage.
     * @tparam T Vertex data type
     */
    template <typename T>
    class VertexBuffer : public Buffer
    {
    public:
        NON_COPIABLE(VertexBuffer)

        VertexBuffer(const std::vector<T>& vertices, const CommandBuffer& command_buffer)
        : Buffer(sizeof(T) * vertices.size(),
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 command_buffer)
        , m_vertices(vertices)
        {
            auto staging_buffer = Buffer::make_staging_buffer(m_size, command_buffer);
            Buffer::set_data(vertices.data(), *staging_buffer, command_buffer);
            Buffer::copy_buffer(*staging_buffer, *this, command_buffer);
        }

        /**
         * @return Amount of vertices in the buffer.
         */
        uint32_t count() const { return m_vertices.size(); }

        /**
         * @return Vertices as they were when creation of the VertexBuffer
         */
        const std::vector<T>& data() const { return m_vertices; }

    private:
        const std::vector<T>& m_vertices;
    };

    /**
     * Buffer for Instance data with VertexBuffer usage.
     * @tparam T Instance data type
     */
    template <typename T>
    class InstanceBuffer : public Buffer {
    public:
        NON_COPIABLE(InstanceBuffer)

        InstanceBuffer(const std::vector<T>& instance_data, const CommandBuffer& command_buffer)
        : Buffer(sizeof(T) * instance_data.size(),
                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal,
                 command_buffer)
        , m_instance_data(instance_data)
        {
            auto staging_buffer = Buffer::make_staging_buffer(m_size, command_buffer);
            Buffer::set_data(instance_data.data(), *staging_buffer, command_buffer);
            Buffer::copy_buffer(*staging_buffer, *this, command_buffer);
        }

        /**
         * @return Instance count.
         */
        uint32_t count() const { return m_instance_data.size(); }

        /**
         * @return Instance data as they were when creation of the buffer.
         */
        const std::vector<T>& data() const { return m_instance_data; }

    private:
        const std::vector<T>& m_instance_data;
    };

    /**
     * @brief Buffer for UniformBuffer usage.
     * @tparam T Uniform buffer object data type
     */
    template <typename T>
    class UniformBuffer : public Buffer {
    public:
        NON_COPIABLE(UniformBuffer)

        explicit UniformBuffer(const CommandBuffer& command_buffer)
        : Buffer(sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 command_buffer) {}

        UniformBuffer(const T& ubo, const CommandBuffer& command_buffer)
        : Buffer(sizeof(T), vk::BufferUsageFlagBits::eUniformBuffer,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 command_buffer)
        {
            auto staging_buffer = Buffer::make_staging_buffer(m_size, command_buffer);
            Buffer::set_data(&ubo, *staging_buffer, command_buffer);
            Buffer::copy_buffer(*staging_buffer, *this, command_buffer);
        }

        /**
         * @brief Update contents of the Uniform Buffer
         */
        void update(const T& ubo) const
        {
            Buffer::set_data(&ubo, *this, m_command_buffer);
        }

        /**
         * @brief Create a Layout Binding at the specified binding for a Uniform Buffer;
         */
        static vk::DescriptorSetLayoutBinding make_binding(uint32_t binding)
        {
            vk::DescriptorSetLayoutBinding ubo_binding;
            ubo_binding.setStageFlags(vk::ShaderStageFlagBits::eVertex);
            ubo_binding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
            ubo_binding.setBinding(binding);
            ubo_binding.setDescriptorCount(1);
            return ubo_binding;
        }
    };

#pragma endregion
}