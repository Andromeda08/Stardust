#pragma once
#include <glm/glm.hpp>

namespace sd::rg
{
    struct pcObject
    {
        glm::mat4 model_matrix {1.0f};
        glm::vec4 color {0.5f};
    };
}