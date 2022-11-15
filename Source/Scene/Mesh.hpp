#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "IData.hpp"
#include "../Resources/Geometry.hpp"
#include "../Vulkan/Device.hpp"
#include "../Vulkan/Command/CommandBuffer.hpp"
#include "../Vulkan/Buffer/IndexBuffer.hpp"
#include "../Vulkan/Buffer/InstanceBuffer.hpp"
#include "../Vulkan/Buffer/VertexBuffer.hpp"
#include "../Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp"

class Mesh {
public:
    Mesh(Geometry* geometry,
         std::vector<IData> instances,
         const CommandBuffer& command_buffers);

    void add_instance(IData instance) { m_instances.push_back(instance); }

    void draw(vk::CommandBuffer &cmd_buffer);

    const std::vector<IData>& instance_data() const { return m_instances; }

    uint32_t instance_count() const { return static_cast<uint32_t>(m_instances.size()); }

public:
    std::unique_ptr<Geometry> m_geometry;
    std::unique_ptr<IndexBuffer> m_index_buffer;
    std::unique_ptr<InstanceBuffer> m_instance_buffer;
    std::unique_ptr<VertexBuffer> m_vertex_buffer;

private:
    // Vulkan device :D
    const Device &m_device;

    // List containing data for the instances of the Mesh
    std::vector<IData> m_instances;
};