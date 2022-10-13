#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;

    static vk::VertexInputBindingDescription binding_description()
    {
        vk::VertexInputBindingDescription description;
        description.setBinding(0);
        description.setStride(sizeof(Vertex));
        description.setInputRate(vk::VertexInputRate::eVertex);
        return description;
    }

    static std::vector<vk::VertexInputAttributeDescription> attribute_description()
    {
        std::vector<vk::VertexInputAttributeDescription> attrs(4);

        // vec3 - [Position, Color, Normal]
        for (size_t i = 0; i < 3; i++)
        {
            attrs[i].setBinding(0);
            attrs[i].setLocation(i);
            attrs[i].setFormat(vk::Format::eR32G32B32A32Sfloat);
        }
        attrs[0].setOffset(offsetof(Vertex, position));
        attrs[1].setOffset(offsetof(Vertex, color));
        attrs[2].setOffset(offsetof(Vertex, normal));

        // vec2 - [UV]
        attrs[3].setBinding(0);
        attrs[3].setLocation(3);
        attrs[3].setFormat(vk::Format::eR32G32Sfloat);
        attrs[3].setOffset(offsetof(Vertex, uv));

        return attrs;
    }
};

const std::vector<Vertex> test_vertices = {
    {{-0.5f, 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0, 1, 0}, {1.0f, 0.0f}},
    {{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0, 1, 0}, {0.0f, 0.0f}},
    {{ 0.5f, 0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0, 1, 0}, {0.0f, 1.0f}},
    {{-0.5f, 0.0f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0, 1, 0}, {1.0f, 1.0f}}
};

const std::vector<uint32_t> test_indices = {
    0, 1, 2, 2, 3, 0
};