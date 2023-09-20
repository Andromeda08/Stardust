#include "NodeFactory.hpp"

#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/DeferredRender.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>

namespace Nebula::RenderGraph::Compiler
{
    std::shared_ptr<Node> NodeFactory::create(NodeType type)
    {
        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return std::make_shared<AmbientOcclusionNode>(m_context.context());
            case NodeType::eDeferredRender:
                return std::make_shared<DeferredRender>(m_context.context());
            case NodeType::eLightingPass:
                return std::make_shared<LightingPass>(m_context.context());
            case NodeType::eSceneProvider:
                return std::make_shared<SceneProviderNode>(m_context.scene());
            default:
                return nullptr;
        }
    }
}