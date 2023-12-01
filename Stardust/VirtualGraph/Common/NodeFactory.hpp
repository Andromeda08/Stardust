#pragma once

#include <memory>
#include "GraphContext.hpp"
#include "NodeType.hpp"

namespace Nebula::RenderGraph
{
    class Node;

    namespace Editor
    {
        class Node;
    }

    class NodeFactory
    {
    public:
        explicit NodeFactory(const RenderGraphContext& graph_context);

        [[nodiscard]] std::shared_ptr<Node> create(const std::shared_ptr<Editor::Node>& en, NodeType type) const;

        [[nodiscard]] static std::shared_ptr<Editor::Node> create_editor(NodeType type);

    private:
        const RenderGraphContext& m_graph_context;
    };
}
