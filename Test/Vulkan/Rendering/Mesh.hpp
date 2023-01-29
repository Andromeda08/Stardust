#pragma once

#include <memory>
#include <Resources/Geometry.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Raytracing/Blas.hpp>

namespace sdvk
{
    class Mesh
    {
    public:
        Mesh(sd::Geometry* p_geometry, CommandBuffers const& command_buffers, Context const& context, std::string const& name = "");

        void draw(vk::CommandBuffer const& command_buffer) const;

        const Buffer& vertex_buffer() const { return *m_vertex_buffer; }

        const Buffer& index_buffer() const { return *m_index_buffer; }

        const sd::Geometry& geometry() const { return *m_geometry; }

        const std::string& name() const { return m_name; }

        const vk::DeviceAddress& blas_address() const { return m_blas->address(); }

    private:
        std::string m_name;

        std::shared_ptr<sd::Geometry> m_geometry;
        std::shared_ptr<Buffer> m_vertex_buffer;
        std::shared_ptr<Buffer> m_index_buffer;
        std::unique_ptr<Blas> m_blas;
    };
}
