#pragma once

#include <memory>
#include "../Buffer/VertexBuffer.hpp"
#include "../Buffer/IndexBuffer.hpp"

struct RtMesh
{
    std::unique_ptr<VertexBuffer> vertex_buffer;
    std::unique_ptr<IndexBuffer> index_buffer;
};