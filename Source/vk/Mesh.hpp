#pragma once

#include <memory>
#include <vk/InstanceData.hpp>
#include <vk/VertexData.hpp>
#include <vk/Buffer.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Resources/Geometry.hpp>

namespace re
{
    class Mesh
    {
        using VB_t = VertexBuffer<VertexData>;
        using IB_t = IndexBuffer<uint32_t>;

    public:
        Mesh(Geometry* geometry, const CommandBuffer& command_buffer)
        : m_geometry(geometry)
        , m_command_buffer(command_buffer)
        {
            m_vertex_buffer = std::make_unique<VB_t>(m_geometry->vertices(), command_buffer);
            m_index_buffer  = std::make_unique<IB_t>(m_geometry->indices(), command_buffer);
        }

        /**
         * @brief Draw a single instance of the Mesh.
         */
        void draw(vk::CommandBuffer& cmd) const
        {
            std::vector<vk::DeviceSize> offsets = { 0 };
            cmd.bindVertexBuffers(0, 1, &m_vertex_buffer->buffer(), offsets.data());
            cmd.bindIndexBuffer(m_index_buffer->buffer(), 0, m_index_buffer->type());
            cmd.drawIndexed(m_index_buffer->count(), 1, 0, 0, 0);
        }

        const VB_t& vertex_buffer() const { return *m_vertex_buffer; }

        const IB_t& index_buffer() const { return *m_index_buffer; }

        const CommandBuffer& command_buffers() const { return m_command_buffer; }

    private:
        std::unique_ptr<Geometry> m_geometry;
        std::unique_ptr<VB_t>     m_vertex_buffer;
        std::unique_ptr<IB_t>     m_index_buffer;

        const CommandBuffer&      m_command_buffer;
    };
}
