#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "IData.hpp"
#include "../Resources/Geometry.hpp"
#include "../Vulkan/Device.hpp"
#include "../Vulkan/Command/CommandBuffers.hpp"
#include "../Vulkan/Buffer/IndexBuffer.hpp"
#include "../Vulkan/Buffer/InstanceBuffer.hpp"
#include "../Vulkan/Buffer/VertexBuffer.hpp"
#include "../Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp"

class Mesh {
public:
    Mesh(Geometry* geometry, std::vector<IData> instances, const vk::Pipeline& pipeline, const Device& device,
         const CommandBuffers& cmds);

    void add_instance(IData instance) { m_instances.push_back(instance); }

    void draw(vk::CommandBuffer &cmd_buffer);

private:
    std::unique_ptr<Geometry> m_geometry;
    std::unique_ptr<IndexBuffer> m_index_buffer;
    std::unique_ptr<InstanceBuffer> m_instance_buffer;
    std::unique_ptr<VertexBuffer> m_vertex_buffer;

    // The pipeline used for rendering the Mesh
    const vk::Pipeline &m_pipeline;

    // Vulkan device :D
    const Device &m_device;

    // List containing data for the instances of the Mesh
    std::vector<IData> m_instances;
};