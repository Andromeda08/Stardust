#pragma once

#include <string>

namespace Nebula::RenderGraph
{
    enum class NodeType
    {
        eAmbientOcclusion,
        eAntiAliasing,
        eDenoise,
        eGaussianBlur,
        eLightingPass,
        ePrePass,
        eRayTracing,

        // Special Types
        ePresent,
        eSceneProvider,
        eUnknown,
    };

    std::string get_node_type_str(NodeType role);
}