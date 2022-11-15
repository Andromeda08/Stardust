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
    };
}
