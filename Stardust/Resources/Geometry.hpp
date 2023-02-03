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

        Geometry(std::vector<sd::VertexData> const& vertices, std::vector<uint32_t> const& indices)
        : m_vertices(vertices), m_indices(indices) {}

        const std::vector<sd::VertexData>& vertices() const { return m_vertices; }

        uint32_t vertex_count() const { return m_vertices.size(); }

        const std::vector<uint32_t>& indices() const { return m_indices; }

        uint32_t index_count() const { return m_indices.size(); }

    protected:
        std::vector<sd::VertexData> m_vertices;
        std::vector<uint32_t>       m_indices;
    };
}