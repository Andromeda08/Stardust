#pragma once

#include <memory>
#include <VirtualGraph/Common/NodeType.hpp>

namespace sd
{
    class Scene;
}

namespace sdvk
{
    class Context;
}

namespace Nebula::RenderGraph
{
    class Node;

    class NodeFactory
    {
    public:
        explicit NodeFactory(const sdvk::Context& context, const std::shared_ptr<sd::Scene>& scene)
        : m_context(context)
        , m_scene(scene)
        {}

        std::shared_ptr<Node> create(NodeType type);

    private:
        std::shared_ptr<sd::Scene> m_scene;
        const sdvk::Context& m_context;
    };
}