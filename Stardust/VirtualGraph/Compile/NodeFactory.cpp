#include "NodeFactory.hpp"

#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/RenderNode.hpp>

namespace Nebula::RenderGraph
{

    std::shared_ptr<Node> NodeFactory::create(NodeType type)
    {
        switch (type)
        {
            case NodeType::eRender:
                return std::make_shared<RenderNode>(m_context);
            // case NodeType::eSceneProvider:
                // return std::make_shared<SceneProviderNode>(m_scene);
            default:
                return nullptr;
        }
    }
}