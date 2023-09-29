#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <VirtualGraph/Common/GraphContext.hpp>
#include <VirtualGraph/Compile/CompileResult.hpp>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>

namespace sd
{
    class Scene;
}

namespace sdvk
{
    class CommandBuffers;
    class Context;
    class Swapchain;
}

namespace Nebula::RenderGraph
{
    class Builder
    {
        using node_ptr = std::shared_ptr<Editor::Node>;
    public:
        explicit Builder(const std::shared_ptr<RenderGraphContext>& rgctx);

        node_ptr& add_pass(NodeType pass_type);

        Builder& make_connection(const node_ptr& start_node, const node_ptr& end_node, const std::string& resource_name);

        Builder& make_connection(const node_ptr& start_node, const node_ptr& end_node,
                                 const std::string& start_resource, const std::string& end_resource);

        Compiler::CompileResult compile();

    private:
        std::shared_ptr<RenderGraphContext> m_ctx;
        std::vector<Editor::Edge> m_edges;
        std::unique_ptr<Editor::VirtualNodeFactory> m_factory;
        std::map<int32_t, node_ptr> m_nodes;
    };
}