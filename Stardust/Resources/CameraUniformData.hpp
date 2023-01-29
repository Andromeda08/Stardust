#pragma once

#include <glm/glm.hpp>

namespace sd
{
    struct CameraUniformData
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 view_inverse;
        glm::mat4 proj_inverse;
        glm::vec4 eye;
    };
}