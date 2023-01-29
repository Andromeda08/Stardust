#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <Resources/Geometry.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>

namespace sdvk
{
    class Blas
    {
    public:
        struct Builder
        {
            Builder() = default;

            Builder& with_geometry(std::shared_ptr<sd::Geometry> const& geometry);

            Builder& with_vertex_buffer(std::shared_ptr<Buffer> const& vertex_buffer);

            Builder& with_index_buffer(std::shared_ptr<Buffer> const& index_buffer);

            Builder& with_name(std::string const& name);

            std::unique_ptr<Blas> create(CommandBuffers const& command_buffers, Context const& context);

        private:
            std::shared_ptr<sd::Geometry> _geometry;
            std::shared_ptr<Buffer> _vertex_buffer;
            std::shared_ptr<Buffer> _index_buffer;
            std::string _name;
        };

        Blas(sd::Geometry const& geometry, Buffer const& vertex_buffer, Buffer const& index_buffer,
             CommandBuffers const& command_buffers, Context const& context);

        const vk::AccelerationStructureKHR& blas() const { return m_blas; }

        const vk::DeviceAddress& address() const { return m_address; }

        const Buffer& buffer() const { return *m_buffer; }

    private:
        vk::AccelerationStructureKHR m_blas;
        vk::DeviceAddress            m_address;
        std::unique_ptr<Buffer>      m_buffer;
    };
}
