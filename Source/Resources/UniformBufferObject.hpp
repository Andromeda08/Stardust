#pragma once

#include <glm/glm.hpp>

struct UniformBufferObject
{
    glm::mat4 view_projection;
    glm::mat4 model;
};