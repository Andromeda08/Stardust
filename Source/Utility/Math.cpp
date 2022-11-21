#include "Math.hpp"

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