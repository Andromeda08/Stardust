#pragma once

#include <glm/glm.hpp>

namespace re
{
    struct VertexData
    {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 uv;

        static vk::VertexInputBindingDescription binding_description(uint32_t binding = 0)
        {
            return { binding, sizeof(VertexData), vk::VertexInputRate::eVertex };
        }

        static std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(uint32_t base_location = 0,
                                                                                       uint32_t binding = 0)
        {
            return {
                { base_location + 0, binding, vk::Format::eR32G32B32Sfloat, 0 },
                { base_location + 1, binding, vk::Format::eR32G32B32Sfloat, offsetof(VertexData, color) },
                { base_location + 2, binding, vk::Format::eR32G32B32Sfloat, offsetof(VertexData, normal) },
                { base_location + 3, binding, vk::Format::eR32G32Sfloat,    offsetof(VertexData, uv) },
            };
        }
    };
}
