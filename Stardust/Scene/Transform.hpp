#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan.hpp>

namespace sd
{
    struct Transform
    {
        glm::vec3 position {0, 0, 0};
        glm::vec3 scale    {1, 1, 1};
        glm::quat rotation = glm::quat();

        glm::mat4 model() const
        {
            auto T = glm::translate(glm::mat4(1.0f), position);
            auto S = glm::scale(glm::mat4(1.0f), scale);
            auto R = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0));
            return T * R * S;
        }

        vk::TransformMatrixKHR model3x4() const
        {
            auto m = model();
            return vk::TransformMatrixKHR({
                std::array<float, 4>{m[0].x, m[1].x, m[2].x, m[3].x},
                std::array<float, 4>{m[0].y, m[1].y, m[2].y, m[3].y},
                std::array<float, 4>{m[0].z, m[1].z, m[2].z, m[3].z}
            });
        }
    };
}