#pragma once

#include <glm/glm.hpp>

namespace re
{
    struct UniformData
    {
        glm::mat4 view_projection;
        glm::mat4 model;
        float     time;
    };
}