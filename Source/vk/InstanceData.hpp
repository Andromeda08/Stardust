#pragma once

#include <glm/glm.hpp>

namespace re
{
    struct InstanceData
    {
        glm::vec3 translate { 0 };
        glm::vec3 scale { 1 };
        glm::vec3 r_axis { 0, 1, 0 };
        float r_angle { 0.0f };
        glm::vec3 color { 0.5f };
        int material_idx { 0 };
        int hit_group { 0 };

        static vk::VertexInputBindingDescription binding_description(uint32_t binding = 1)
        {
            return { binding, sizeof(InstanceData), vk::VertexInputRate::eInstance };
        }
    };
}
