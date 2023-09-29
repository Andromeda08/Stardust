#pragma once

#include <memory>
#include <VirtualGraph/Common/NodeType.hpp>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Editor/Node.hpp>

namespace sd
{
    class Scene;
}

namespace sdvk
{
    class Context;
    class Swapchain;
}

namespace Nebula::RenderGraph
{
    class Node;
}

namespace Nebula::RenderGraph::Compiler
{
    class NodeFactory
    {
    public:
        explicit NodeFactory(const RenderGraphContext& context)
        : m_context(context)
        {}

        std::shared_ptr<Node> create(const std::shared_ptr<Editor::Node>& enode, NodeType type);

    private:
        const RenderGraphContext& m_context;
    };
}