#pragma once

#include <vk/Buffer.hpp>
#include <vk/InstanceData.hpp>
#include <vk/Mesh.hpp>

namespace re
{
    class InstancedGeometry
    {
    public:
        InstancedGeometry(Geometry* geometry, const std::vector<InstanceData>& instance_data, const CommandBuffer& command_buffer)
        : m_instance_data(instance_data)
        {
            m_mesh = std::make_unique<Mesh>(geometry, command_buffer);
            m_instance_buffer = std::make_unique<InstanceBuffer<InstanceData>>(instance_data, command_buffer);
        }

        InstancedGeometry(Mesh* mesh, const std::vector<InstanceData>& instance_data)
        : m_instance_data(instance_data)
        , m_mesh(mesh)
        {
            m_instance_buffer = std::make_unique<InstanceBuffer<InstanceData>>(instance_data, mesh->command_buffers());
        }

        /**
         * @brief Draw a single instance of the Mesh.
         */
        void draw_mesh(vk::CommandBuffer cmd) const
        {
            m_mesh->draw(cmd);
        }

        /**
         * @brief Draw all instances.
         */
        void draw_instanced(vk::CommandBuffer cmd) const
        {
            std::vector<vk::DeviceSize> offsets = { 0 };
            cmd.bindVertexBuffers(0, 1, &m_mesh->vertex_buffer().buffer(), offsets.data());
            cmd.bindVertexBuffers(1, 1, &m_instance_buffer->buffer(), offsets.data());
            cmd.bindIndexBuffer(m_mesh->index_buffer().buffer(), 0, m_mesh->index_buffer().type());
            cmd.drawIndexed(m_mesh->index_buffer().count(), m_instance_data.size(), 0, 0, 0);
        }

    private:
        std::unique_ptr<Mesh>                         m_mesh;
        std::unique_ptr<InstanceBuffer<InstanceData>> m_instance_buffer;
        const std::vector<InstanceData>&              m_instance_data;
    };
}