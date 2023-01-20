#pragma once

#include <cstdint>
#include <vector>
#include <Resources/VertexData.hpp>

namespace sd
{
    class Geometry
    {
    public:
        Geometry() = default;

        const std::vector<sd::VertexData>& vertices() { return m_vertices; }

        const std::vector<uint32_t>& indices() { return m_indices; }

    protected:
        std::vector<sd::VertexData> m_vertices;
        std::vector<uint32_t>       m_indices;
    };
}