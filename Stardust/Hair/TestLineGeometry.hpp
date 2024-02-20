#pragma once

#include <cstdint>
#include <vector>
#include <Resources/Geometry.hpp>
#include <Resources/VertexData.hpp>

namespace Nebula
{
    using namespace sd;

    class Strand : public Geometry
    {
    public:
        explicit Strand(const int32_t length = 4,
                        const glm::vec3& axis = { 1, 0, 0 })
        : Geometry()
        {
            for (int32_t i = 0; i < length; i++)
            {
                VertexData vertex {};
                vertex.position = axis * static_cast<float>(i);
                vertex.position.y -= glm::sqrt(static_cast<float>(i));
                vertex.normal = glm::vec3(0);
                vertex.uv = glm::vec2(0);
                m_vertices.push_back(vertex);
            }

            m_indices = { 0, 1, 2, 3 };
        }
    };
}