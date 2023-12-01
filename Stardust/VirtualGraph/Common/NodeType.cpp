#include "NodeType.hpp"

namespace Nebula::RenderGraph
{
    std::string get_node_type_str(const NodeType type)
    {
        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return "Ambient Occlusion";
            case NodeType::eAntiAliasing:
                return "Anti-Aliasing";
            case NodeType::eBloom:
                return "Bloom";
            case NodeType::eGBufferPass:
                return "Deferred Pass";
            case NodeType::eDenoise:
                return "Denoise";
            case NodeType::eGaussianBlur:
                return "Blur";
            case NodeType::eLightingPass:
                return "Lighting Pass";
            case NodeType::eMeshShaderGBufferPass:
                return "Mesh Shader G-Buffer Pass";
            case NodeType::eRayTracing:
                return "Ray Tracing";
            case NodeType::ePresent:
                return "Present";
            case NodeType::eSceneProvider:
                return "Scene Provider";
            case NodeType::eUnknown:
                // Falls Through
            default:
                return "Unknown";
        }
    }

    std::vector<NodeType> get_node_types()
    {
        static std::vector nodes = {
            NodeType::eAmbientOcclusion, NodeType::eAntiAliasing, NodeType::eGBufferPass,
            NodeType::eGaussianBlur, NodeType::eLightingPass, NodeType::eMeshShaderGBufferPass,
            NodeType::eRayTracing,
        };

        return nodes;
    }
}
