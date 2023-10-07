#pragma once

#include <format>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <queue>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>

namespace Nebula::RenderGraph::Algorithm
{
    struct Range
    {
        int32_t start;
        int32_t end;

        bool overlaps(const Range& other) const
        {
            return std::max(start, other.start) <= std::min(end, other.end);
        }
    };

    // Helper struct
    struct IntResourceUserInfo
    {
        int32_t      node_id;               // ID of the Node that consumes a resource
        int32_t      node_idx;              // Index of the Node in the execution order
        std::string  node_name;             // Name of the consumer Node
        int32_t      res_id;                // ID of the ResourceDescription of the consumer Node
        std::string  res_name;              // Name of the Resource at the consumer Node
        ResourceRole role;                  // Role of the Resource at the consumer Node
        std::shared_ptr<Editor::Node> node; // Consumer Node
    };

    // Helper struct
    struct IntResourceInfo
    {
        int32_t      origin_node_id;    // ID of the Node that outputs this resource
        int32_t      origin_node_idx;   // Index of the Node in the execution order
        std::string  origin_node_name;  // Name of the origin Node
        int32_t      origin_res_id;     // ID of the ResourceDescription associated with the resources
        std::string  origin_res_name;   // Name of the Resource at the origin Node
        ResourceType type;              // Resource Type
        ResourceRole role;              // Resource Role (Input / Output)
        bool         optimizable;       // Is the resource optimizable
        Editor::ResourceDescription rd; // Original resource description

        std::vector<IntResourceUserInfo> users;
    };

    // Helper struct
    struct IntOptimizerResourceUsagePoint
    {
        int32_t      point;
        int32_t      user_res_id;
        std::string  used_as;
        int32_t      user_node_id;
        std::string  used_by;
        ResourceRole role;
    };

    /**
     * Optimizer created Resource
     * The information contained in the struct is enough to know mappings between original and optimizer resources:
     * the list of IntOptimizerResourceUsagePoint structs contain the required information (user_node_id, user_res_id)
     * to map them to the appropriate OptimizerResource using its "id" variable.
     */
    struct OptimizerResource
    {
        int32_t           id;
        std::set<int32_t> usage_points;
        IntResourceInfo   original_desc;
        ResourceType      type;

        // Ensure image format compatibility
        vk::Format          format;
        vk::Extent2D        extent;
        vk::ImageUsageFlags usage_flags;

        std::vector<IntOptimizerResourceUsagePoint> usage_point_meta;

        Range get_usage_range()
        {
            return {
                .start = *std::begin(usage_points),
                .end   = *std::end(usage_points),
            };
        }
    };

    struct ResourceOptimizationResult
    {
        int32_t non_optimizable_count {0};
        int32_t optimized_resource_count {0};
        int32_t original_resource_count {0};
        std::vector<OptimizerResource> resources;
    };

    class ResourceOptimizer
    {
    public:
        ResourceOptimizer(const std::vector<std::shared_ptr<Editor::Node>>& nodes, const std::vector<Editor::Edge>& edges)
        : m_nodes(nodes), m_edges(edges)
        {
        }

        ResourceOptimizationResult run()
        {
            auto R = evaluate_required_resources();
            std::vector<OptimizerResource> opt_resources;

            int32_t non_optimizable_count {0};
            for (const auto& ri : R)
            {
                std::set<int32_t> usage_points { ri.origin_node_idx };
                std::vector<IntOptimizerResourceUsagePoint> usage_points_meta {{ri.origin_node_idx, ri.origin_res_id, ri.origin_res_name, ri.origin_node_id, ri.origin_node_name, ri.role }};
                for (const auto& user : ri.users)
                {
                    usage_points.insert(user.node_idx);
                    usage_points_meta.emplace_back(user.node_idx, user.res_id, user.res_name, user.node_id, user.node_name, user.role);
                }

                OptimizerResource ior {
                    .id = m_id_sequence++,
                    .usage_points = usage_points,
                    .original_desc = ri,
                    .type = ri.type,
                    .format = ri.rd.spec.format,
                    .extent = ri.rd.spec.extent,
                    .usage_flags = ri.rd.spec.usage_flags,
                    .usage_point_meta = usage_points_meta,
                };

                // Case: Add non-optimizable resource
                if (!ri.optimizable)
                {
                    opt_resources.push_back(ior);
                    non_optimizable_count++;
                    continue;
                }

                // Case: No resources
                if (opt_resources.empty())
                {
                    opt_resources.push_back(ior);
                    continue;
                }

                // Case: Try inserting into existing resource
                bool was_inserted = false;
                for (auto& opt_res : opt_resources)
                {
                    Range current_range = opt_res.get_usage_range();
                    Range incoming_range = { *std::begin(usage_points), *std::end(usage_points) };

                    bool can_accommodate = !current_range.overlaps(incoming_range)
                                        && ri.type == ior.type
                                        && ri.rd.spec.format == ior.format
                                        && ri.rd.spec.extent == ior.extent
                                        && ri.rd.spec.usage_flags == ior.usage_flags;

                    if (can_accommodate)
                    {
                        for (const auto& pt : usage_points)
                        {
                            if (opt_res.usage_points.contains(pt))
                            {
                                auto message = std::format("[Error] Resource is already used at point {}!", pt);
                                std::cout << message << std::endl;
                                // throw std::runtime_error(message);
                            }

                            opt_res.usage_points.insert(pt);

                            auto iter = std::find_if(std::begin(usage_points_meta), std::end(usage_points_meta), [&](const auto& p){
                                return p.point == pt;
                            });
                            opt_res.usage_point_meta.push_back(*iter);
                        }

                        was_inserted = true;
                        break;
                    }
                }

                // Case: Failed to insert, add new resource
                if (!was_inserted)
                {
                    opt_resources.push_back(ior);
                }
            }

            ResourceOptimizationResult result {
                .non_optimizable_count = non_optimizable_count,
                .optimized_resource_count = static_cast<int32_t>(opt_resources.size()),
                .original_resource_count = static_cast<int32_t>(R.size()),
                .resources = opt_resources,
            };

            return result;
        }

    private:
        std::vector<IntResourceInfo> evaluate_required_resources()
        {
            std::vector<IntResourceInfo> required_resources;

            // Find output resources
            for (int32_t i = 0; i < m_nodes.size(); i++)
            {
                const auto& node = m_nodes[i];
                const auto& resource_descriptions = node->resources();
                for (const auto& resource : resource_descriptions)
                {
                    if (resource.role == ResourceRole::eInput) continue;

                    IntResourceInfo desc {
                        .origin_node_id = node->id(),
                        .origin_node_idx = i,
                        .origin_node_name = node->name(),
                        .origin_res_id = resource.id,
                        .origin_res_name = resource.name,
                        .type = resource.type,
                        .role = resource.role,
                        .optimizable = m_optimizable_types.contains(resource.type),
                        .rd = resource,
                        .users = {},
                    };
                    required_resources.push_back(desc);
                }
            }

            // Find resource consumers
            for (auto& ri : required_resources)
            {
                for (const auto& edge : m_edges)
                {
                    if (ri.origin_node_id != edge.start.node_id) continue;
                    if (ri.origin_node_id == edge.end.node_id) continue;
                    if (ri.origin_res_id != edge.start.res_id) continue;

                    // atp: non-origin node, edge starts from current resource
                    int32_t consumer_id = edge.end.node_id;
                    int32_t consumer_res_id = edge.end.res_id;

                    auto iter = std::find_if(std::begin(m_nodes), std::end(m_nodes), [&](const auto& node){
                        return node->id() == consumer_id;
                    });

                    auto consumer_node_idx = static_cast<int32_t>(std::distance(std::begin(m_nodes), iter));
                    auto& consumer_node = m_nodes[consumer_node_idx];

                    IntResourceUserInfo rui {
                        .node_id = consumer_id,
                        .node_idx = consumer_node_idx,
                        .node_name = consumer_node->name(),
                        .res_id = consumer_res_id,
                        .res_name = edge.end.res_name,
                        .role = consumer_node->get_resource(consumer_res_id).role,
                        .node = consumer_node,
                    };

                    ri.users.push_back(rui);
                }
            }

            return required_resources;
        }

    private:
        int32_t m_id_sequence {0};

        const std::set<ResourceType> m_optimizable_types { ResourceType::eDepthImage, ResourceType::eImage };

        const std::vector<std::shared_ptr<Editor::Node>>& m_nodes;  // Nodes in execution order
        const std::vector<Editor::Edge>& m_edges;                   // Original graph edges
    };
}