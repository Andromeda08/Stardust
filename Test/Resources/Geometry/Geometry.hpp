#pragma once

#include <cstdint>
#include <vector>
#include <Resources/VertexData.hpp>

namespace sdr
{
    class Geometry
    {
    public:
        Geometry() = default;

        const std::vector<VertexData>& vertices() { return m_vertices; }

        const std::vector<uint32_t>& indices() { return m_indices; }

    private:
        virtual std::vector<VertexData> gen_vertices() = 0;

        virtual std::vector<uint32_t> gen_indices() = 0;

        std::vector<VertexData> m_vertices;
        std::vector<uint32_t>   m_indices;
    };
}