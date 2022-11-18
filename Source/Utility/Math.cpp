#include "Math.hpp"

namespace Math
{
    glm::mat4 model(std::optional<glm::vec3> translate,
                    std::optional<glm::vec3> scale,
                    std::optional<glm::vec3> r_axis,
                    std::optional<float> r_angle)
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

std::ostream& operator<<(std::ostream& os, const vk::TransformMatrixKHR& rhs)
{
    auto& m = rhs.matrix;
    for (size_t i = 0 ; i < 3; i++)
    {
        auto& row = m[i];
        os << "[ ";
        for (size_t j = 0 ; j < 4; j++)
        {
            os << row[j];
            (j == 3) ? os << " ]\n" : os << ", ";
        }
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const glm::mat4& rhs)
{
    for (int i = 0; i < 4; i++)
    {
        os << "[ " << rhs[i].x << ", " << rhs[i].y << ", " << rhs[i].z << ", "<< rhs[i].w << " ]\n";
    }
    return os;
}