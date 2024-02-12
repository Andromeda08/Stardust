#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/gtx/hash.hpp>

namespace sd
{
    struct VertexData
    {
        glm::vec3 position;
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
                { base_location + 1, binding, vk::Format::eR32G32B32Sfloat, offsetof(VertexData, normal) },
                { base_location + 2, binding, vk::Format::eR32G32Sfloat,    offsetof(VertexData, uv) },
            };
        }

        bool operator==(const VertexData& rhs) const
        {
            return position == rhs.position && normal == rhs.normal && uv == rhs.uv;
        }
    };
}

namespace std {
    template<> struct hash<sd::VertexData> {
        size_t operator()(const sd::VertexData& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                     (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

