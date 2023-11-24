#include "Mesh.hpp"
#include <format>

namespace sdvk
{
    Mesh::Mesh(sd::Geometry* p_geometry, const CommandBuffers& command_buffers, const Context& context, const std::string& name)
    : m_geometry(p_geometry), m_name(name)
    {
        m_vertex_buffer = Buffer::Builder()
            .with_name(std::format("[Mesh] {} - Vertex Buffer", name))
            .with_size(sizeof(sd::VertexData) * m_geometry->vertices().size())
            .as_vertex_buffer()
            .create_with_data(m_geometry->vertices().data(), command_buffers, context);

        m_index_buffer = Buffer::Builder()
            .with_name(std::format("[Mesh] {} - Index Buffer", name))
            .with_size(sizeof(uint32_t) * m_geometry->indices().size())
            .as_index_buffer()
            .create_with_data(m_geometry->indices().data(), command_buffers, context);

        if (context.is_raytracing_capable())
        {
            m_blas = Blas::Builder()
                .with_geometry(m_geometry)
                .with_vertex_buffer(m_vertex_buffer)
                .with_index_buffer(m_index_buffer)
                .with_name(std::format("[Mesh] {} - BLAS", name))
                .create(command_buffers, context);
        }

        create_meshlets();
        m_meshlets_size = m_meshlets.size();
        m_meshlet_buffer = Buffer::Builder()
            .with_size(sizeof(Meshlet) * m_meshlets.size())
            .as_storage_buffer()
            .with_name(std::format("[Mesh] {} - Meshlets", name))
            .create_with_data(m_meshlets.data(), command_buffers, context);
    }

    void Mesh::draw(const vk::CommandBuffer& command_buffer) const
    {
        static const std::vector<vk::DeviceSize> offsets = { 0 };
        command_buffer.bindVertexBuffers(0, 1, &m_vertex_buffer->buffer(), offsets.data());
        command_buffer.bindIndexBuffer(m_index_buffer->buffer(), 0, vk::IndexType::eUint32);
        command_buffer.drawIndexed(m_geometry->indices().size(), 1, 0, 0, 0);
    }

    void Mesh::draw_mesh_tasks(const vk::CommandBuffer& command_buffer) const
    {
        command_buffer.drawMeshTasksEXT(m_meshlets_size, 1, 1);
    }

    void Mesh::create_meshlets(const uint32_t max_vertices, const uint32_t max_indices)
    {
        const auto& geom = *m_geometry;
        const auto& indices = geom.indices();

        std::vector<Meshlet> meshlets;
        std::vector<uint8_t> meshlet_vertices(geom.vertex_count(), 0xff);
        Meshlet meshlet = {};

        for (uint32_t i = 0; i < geom.index_count(); i += 3)
        {
            const uint32_t a = indices[i + 0];
            const uint32_t b = indices[i + 1];
            const uint32_t c = indices[i + 2];

            uint8_t& av = meshlet_vertices[a];
            uint8_t& bv = meshlet_vertices[b];
            uint8_t& cv = meshlet_vertices[c];

            if (meshlet.vertex_count + (av == 0xff) + (bv == 0xff) + (cv == 0xff) > max_vertices
                || meshlet.index_count + 3 > max_indices)
            {
                meshlets.push_back(meshlet);

                for (uint32_t j = 0; j < meshlet.vertex_count; j++)
                {
                    meshlet_vertices[meshlet.vertex[j]] = 0xff;
                }

                meshlet = {};
            }

            if (av == 0xff)
            {
                av = meshlet.vertex_count;
                meshlet.vertex[meshlet.vertex_count++] = a;
            }

            if (bv == 0xff)
            {
                bv = meshlet.vertex_count;
                meshlet.vertex[meshlet.vertex_count++] = b;
            }

            if (cv == 0xff)
            {
                cv = meshlet.vertex_count;
                meshlet.vertex[meshlet.vertex_count++] = c;
            }

            meshlet.index[meshlet.index_count++] = av;
            meshlet.index[meshlet.index_count++] = bv;
            meshlet.index[meshlet.index_count++] = cv;
        }

        if (meshlet.index_count > 0)
        {
            meshlets.push_back(meshlet);
        }

        m_meshlets = meshlets;
    }
}
