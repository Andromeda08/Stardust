#include "NodeFactory.hpp"

#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/BlurNode.hpp>
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
            case NodeType::eAntiAliasing:
                return std::make_shared<AntiAliasingNode>(m_context.context());
            case NodeType::eGaussianBlur:
                return std::make_shared<BlurNode>(m_context.context());
            case NodeType::eDeferredRender:
                return std::make_shared<DeferredRender>(m_context.context());
            case NodeType::eLightingPass:
                return std::make_shared<LightingPass>(m_context.context());
            case NodeType::eSceneProvider:
                return std::make_shared<SceneProviderNode>(m_context.scene());
            case NodeType::ePresent:
                return std::make_shared<PresentNode>(m_context.context(), m_context.swapchain());

            default:
                return nullptr;
        }
    }
}