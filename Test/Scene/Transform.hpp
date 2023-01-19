#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan.hpp>

namespace sd
{
    struct Transform
    {
        glm::vec3 position;
        glm::vec3 scale;
        glm::quat rotation;

        glm::mat4 model() const
        {
            auto M = glm::mat4(1.0f);
            M = glm::toMat4(rotation) * M;
            M = glm::scale(M, scale);
            M = glm::translate(M, position);
            return M;
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