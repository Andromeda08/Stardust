#pragma once

#include <glm/glm.hpp>
#include "../Utility/Math.hpp"

struct IData
{
    glm::vec3 translate{0};
    glm::vec3 scale{1};
    glm::vec3 rotation_axis{0, 1, 0};
    float rotation_angle{0.0f};
    glm::vec3 color{0.5f};
};