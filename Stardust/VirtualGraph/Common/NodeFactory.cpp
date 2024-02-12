#include "NodeFactory.hpp"

#include <Nebula/Utility.hpp>
#include <VirtualGraph/Editor/Node.hpp>
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
#include <VirtualGraph/RenderGraph/Nodes/HairRenderer.hpp>

namespace Nebula::RenderGraph
{
    NodeFactory::NodeFactory(const RenderGraphContext& graph_context): m_graph_context(graph_context) {}

    std::shared_ptr<Node> NodeFactory::create(const std::shared_ptr<Editor::Node>& en, const NodeType type) const
    {
        const auto& ctx = m_graph_context.context();

        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return std::make_shared<AmbientOcclusionNode>(ctx);
            case NodeType::eAntiAliasing:
                return std::make_shared<AntiAliasingNode>(ctx);
            case NodeType::eGaussianBlur:
                return std::make_shared<BlurNode>(ctx);
            case NodeType::eLightingPass:
                return std::make_shared<LightingPass>(ctx, dynamic_cast<Editor::LightingPassNode&>(*en).params);
            case NodeType::eGBufferPass:
                return std::make_shared<GBufferPass>(ctx);
            case NodeType::eMeshShaderGBufferPass:
                return std::make_shared<MeshGBufferPass>(ctx, dynamic_cast<Editor::MeshGBufferPassEditorNode&>(*en).m_params);
            case NodeType::eRayTracing:
                return std::make_shared<RayTracingNode>(ctx, dynamic_cast<Editor::RayTracingNode&>(*en).params);
            case NodeType::eHairRenderer:
                return std::make_shared<HairRenderer>(ctx);
            case NodeType::ePresent:
                return std::make_shared<PresentNode>(ctx, m_graph_context.swapchain(), dynamic_cast<Editor::PresentNode&>(*en).params);
            case NodeType::eSceneProvider:
                return std::make_shared<SceneProviderNode>(m_graph_context.scene());
            case NodeType::eUnknown:
                default:
                throw Utility::make_exception(std::format("Unsupported node type: \"{}\"", get_node_type_str(type)));
        }
    }

    std::shared_ptr<Editor::Node> NodeFactory::create_editor(const NodeType type)
    {
        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return std::make_shared<Editor::AmbientOcclusionNode>();
            case NodeType::eAntiAliasing:
                return std::make_shared<Editor::AntiAliasingNode>();
            case NodeType::eGaussianBlur:
                return std::make_shared<Editor::BlurNode>();
            case NodeType::eGBufferPass:
                return std::make_shared<Editor::GBufferPass>();
            case NodeType::eLightingPass:
                return std::make_shared<Editor::LightingPassNode>();
            case NodeType::eMeshShaderGBufferPass:
                return std::make_shared<Editor::MeshGBufferPassEditorNode>();
            case NodeType::ePresent:
                return std::make_shared<Editor::PresentNode>();
            case NodeType::eRayTracing:
                return std::make_shared<Editor::RayTracingNode>();
            case NodeType::eHairRenderer:
                return std::make_shared<Editor::HairRendererEditorNode>();
            case NodeType::eSceneProvider:
                return std::make_shared<Editor::SceneProviderNode>();
            case NodeType::eUnknown:
            default:
                throw Utility::make_exception(std::format("Unsupported node type: \"{}\"", get_node_type_str(type)));
        }
    }
}
