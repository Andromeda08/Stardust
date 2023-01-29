#pragma once

#include "Transform.hpp"
#include <Vulkan/Rendering/Mesh.hpp>

namespace sd
{
    struct Object
    {
        Transform transform;
        sdvk::Mesh const& mesh;
    };
}