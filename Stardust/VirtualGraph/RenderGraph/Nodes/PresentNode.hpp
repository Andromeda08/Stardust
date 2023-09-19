#pragma once

#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>

namespace Nebula::RenderGraph
{
    class PresentNode : public Node
    {
    public:
        static const std::vector<ResourceSpecification> s_resource_specs;
        const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; }

    public:

    private:

    };
}