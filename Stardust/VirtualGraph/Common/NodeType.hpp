#pragma once

#include <string>
#include <vector>

namespace Nebula::RenderGraph
{
    enum class NodeType
    {
        eAmbientOcclusion,
        eAntiAliasing,
        eBloom,
        eDenoise,
        eGaussianBlur,
        eLightingPass,
        eMeshShaderGBufferPass,
        ePrePass,
        eRayTracing,

        // Special Types
        ePresent,
        eSceneProvider,
        eUnknown,
    };

    std::string get_node_type_str(NodeType role);

    std::vector<NodeType> get_node_types();
}
