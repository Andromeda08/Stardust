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
    };
}
