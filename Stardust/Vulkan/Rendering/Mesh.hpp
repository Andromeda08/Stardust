#pragma once

#include <memory>
#include <Resources/Geometry.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Raytracing/Blas.hpp>

namespace sdvk
{
    struct Meshlet
    {
        uint32_t vertex[64]{};
        uint32_t index[126]{};
        uint32_t vertex_count {0};
        uint32_t index_count {0};
    };

    class Mesh
    {
    public:
        Mesh(sd::Geometry* p_geometry, const CommandBuffers& command_buffers, const Context& context, const std::string& name = "");

        void draw(const vk::CommandBuffer& command_buffer) const;

        void draw_mesh_tasks(const vk::CommandBuffer& command_buffer) const;

        const Buffer& vertex_buffer() const { return *m_vertex_buffer; }

        const Buffer& index_buffer() const { return *m_index_buffer; }

        const Buffer& meshlet_buffer() const { return *m_meshlet_buffer; }

        const std::vector<Meshlet>& meshlet_data() const { return m_meshlets; }

        const sd::Geometry& geometry() const { return *m_geometry; }

        const std::string& name() const { return m_name; }

        const vk::DeviceAddress& blas_address() const { return m_blas->address(); }

    private:
        void create_meshlets(uint32_t max_vertices = 64, uint32_t max_indices = 126);

        std::string                   m_name;
        uint32_t                      m_meshlets_size;
        std::vector<Meshlet>          m_meshlets;
        std::shared_ptr<sd::Geometry> m_geometry;
        std::shared_ptr<Buffer>       m_vertex_buffer;
        std::shared_ptr<Buffer>       m_index_buffer;
        std::shared_ptr<Buffer>       m_meshlet_buffer;
        std::unique_ptr<Blas>         m_blas;
    };
}
