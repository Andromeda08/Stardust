#pragma once

#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

// Namespace containing Math utilities
namespace Math
{
    /**
     * @brief Returns the appropriate 3x4 row major matrix used by Vulkan
     * from the given 4x4 column major GLM matrix.
     */
    static vk::TransformMatrixKHR glmToKhr(glm::mat4 matrix) {
        auto m = glm::transpose(matrix);
        return vk::TransformMatrixKHR({
            std::array<float, 4>{ m[0].x, m[0].y, m[0].z, m[0].w },
            std::array<float, 4>{ m[1].x, m[1].y, m[1].z, m[1].w },
            std::array<float, 4>{ m[2].x, m[2].y, m[2].z, m[2].w }
        });
    }

    /**
     * @brief Returns the appropriate model matrix with the given transformations.
     * @param translate Translation
     * @param scale Scale modifiers
     * @param r_axis Axis of rotation
     * @param r_angle Angle of rotation
     */
    static glm::mat4 model(std::optional<glm::vec3> translate = std::nullopt,
                           std::optional<glm::vec3> scale     = std::nullopt,
                           std::optional<glm::vec3> r_axis    = std::nullopt,
                           std::optional<float>     r_angle   = std::nullopt)
    {
        auto result = glm::mat4(1.0f);

        if (scale.has_value())
            result = glm::scale(result, translate.value());

        if (r_axis.has_value() && r_angle.has_value())
            result = glm::rotate(result, r_angle.value(), r_axis.value());

        if (translate.has_value())
            result = glm::translate(result, translate.value());

        return result;
    }
}