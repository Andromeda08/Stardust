#include "NodeFactory.hpp"

#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/BlurNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PrePass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RayTracingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>

namespace Nebula::RenderGraph::Compiler
{
    std::shared_ptr<Node> NodeFactory::create(const std::shared_ptr<Editor::Node>& editor_node, NodeType type)
    {
        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return std::make_shared<AmbientOcclusionNode>(m_context.context());
            case NodeType::eAntiAliasing:
                return std::make_shared<AntiAliasingNode>(m_context.context());
            case NodeType::eGaussianBlur:
                return std::make_shared<BlurNode>(m_context.context());
            case NodeType::ePrePass:
                return std::make_shared<PrePass>(m_context.context());
            case NodeType::eLightingPass:
                return std::make_shared<LightingPass>(m_context.context(),
                                                      dynamic_cast<Editor::LightingPassNode&>(*editor_node).params);
            case NodeType::eSceneProvider:
                return std::make_shared<SceneProviderNode>(m_context.scene());
            case NodeType::ePresent:
                return std::make_shared<PresentNode>(m_context.context(), m_context.swapchain());

            default:
                return nullptr;
        }
    }
}