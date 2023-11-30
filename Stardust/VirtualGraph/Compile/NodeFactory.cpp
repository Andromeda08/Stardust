#include "NodeFactory.hpp"

#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/BlurNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/GBufferPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/LightingPass.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RayTracingNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/PresentNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/MeshGBufferPass.hpp>

namespace Nebula::RenderGraph::Compiler
{
    std::shared_ptr<Node> NodeFactory::create(const std::shared_ptr<Editor::Node>& editor_node, const NodeType type)
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
                return std::make_shared<GBufferPass>(m_context.context());
            case NodeType::eLightingPass:
                return std::make_shared<LightingPass>(m_context.context(),
                                                      dynamic_cast<Editor::LightingPassNode&>(*editor_node).params);
            case NodeType::eMeshShaderGBufferPass:
                return std::make_shared<MeshGBufferPass>(m_context.context(),
                                                         dynamic_cast<Editor::MeshGBufferPassEditorNode&>(*editor_node).m_params);
            case NodeType::eSceneProvider:
                return std::make_shared<SceneProviderNode>(m_context.scene());
            case NodeType::eRayTracing:
                return std::make_shared<RayTracingNode>(m_context.context(),
                                                        dynamic_cast<Editor::RayTracingNode&>(*editor_node).params);
            case NodeType::ePresent:
                return std::make_shared<PresentNode>(m_context.context(), m_context.swapchain(),
                                                     dynamic_cast<Editor::PresentNode&>(*editor_node).params);

            default:
                return nullptr;
        }
    }
}
