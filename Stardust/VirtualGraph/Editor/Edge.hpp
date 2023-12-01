#pragma once

#include <cstdint>
#include <string>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/Common/ResourceType.hpp>

namespace Nebula::RenderGraph::Editor
{
    struct Edge
    {
    private:
        struct Point
        {
            int32_t node_id, res_id;
            std::string node_name, res_name;

            Point(const Node& n, const ResourceDescription& r)
            : node_id(n.id())
            , res_id(r.id)
            , node_name(n.name())
            , res_name(r.name)
            {}
        };

    public:
        int32_t id = sd::util::gen_id();
        Point start, end;
        ResourceType attr_type;

        Edge(const Node& node_start, const ResourceDescription& resource_start,
             const Node& node_end, const ResourceDescription& resource_end,
             ResourceType attribute_type)
        : start(node_start, resource_start)
        , end(node_end, resource_end)
        , attr_type(attribute_type)
        {}
    };
}