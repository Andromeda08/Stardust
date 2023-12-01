#pragma once

#include <bitset>
#include <format>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>

#define NEBULA_OPT_DEBUG
// #define NEBULA_OPT_DEBUG_VERBOSE

#ifdef NEBULA_OPT_DEBUG_VERBOSE
#include <iostream>
#include <sstream>
#endif

namespace Nebula::RenderGraph::Algorithm
{
    // Internal helper struct
    struct IntResourceUserInfo
    {
        int32_t      node_id;               // ID of the Node that consumes a resource
        int32_t      node_idx;              // Index of the Node in the execution order
        std::string  node_name;             // Name of the consumer Node
        int32_t      res_id;                // ID of the ResourceDescription of the consumer Node
        std::string  res_name;              // Name of the Resource at the consumer Node
        ResourceRole role;                  // Role of the Resource at the consumer Node
        std::shared_ptr<Editor::Node> node; // Consumer Node pointer
    };

    // Internal helper struct
    struct IntResourceInfo
    {
        int32_t      origin_node_id {};             // ID of the Node that outputs this resource
        int32_t      origin_node_idx {};            // Index of the Node in the execution order
        std::string  origin_node_name;              // Name of the origin Node
        int32_t      origin_res_id {};              // ID of the ResourceDescription associated with the resources
        std::string  origin_res_name;               // Name of the Resource at the origin Node
        ResourceType type {ResourceType::eUnknown}; // Resource Type
        ResourceRole role {ResourceRole::eUnknown}; // Resource Role (Input / Output)
        bool         optimizable {false};           // Is the resource optimizable
        Editor::ResourceDescription rd;             // Original resource description=
        std::vector<IntResourceUserInfo> users;     // List of users
    };

    // Internal helper struct
    struct IntOptimizerResourceUsagePoint
    {
        int32_t      point {};
        int32_t      user_res_id {-1};
        std::string  used_as;
        int32_t      user_node_id {-1};
        std::string  used_by;
        ResourceRole role {ResourceRole::eUnknown};

        IntOptimizerResourceUsagePoint() = default;

        explicit IntOptimizerResourceUsagePoint(const IntResourceInfo& iri)
        {
            point = iri.origin_node_idx;
            user_res_id = iri.origin_res_id;
            used_as = iri.origin_res_name;
            user_node_id = iri.origin_node_id;
            used_by = iri.origin_node_name;
            role = iri.role;
        }

        explicit IntOptimizerResourceUsagePoint(const IntResourceUserInfo& rui)
        {
            point = rui.node_idx;
            user_res_id = rui.res_id;
            used_as = rui.res_name;
            user_node_id = rui.node_id;
            used_by = rui.node_name;
            role = rui.role;
        }
    };

    inline bool operator<(const IntOptimizerResourceUsagePoint& lhs, const IntOptimizerResourceUsagePoint& rhs)
    {
        return lhs.point < rhs.point;
    }

    inline bool operator==(const IntOptimizerResourceUsagePoint& lhs, const IntOptimizerResourceUsagePoint& rhs)
    {
        return lhs.point == rhs.point;
    }

    struct Range
    {
        int32_t start;
        int32_t end;

        explicit Range(const std::set<IntOptimizerResourceUsagePoint>& points)
        {
            const auto min = std::min_element(std::begin(points), std::end(points));
            const auto max = std::max_element(std::begin(points), std::end(points));

            start = min->point;
            end = max->point;

            validate();
        }

        Range(int32_t a, int32_t b): start(a), end(b)
        {
            validate();
        }

        bool overlaps(const Range& other) const
        {
            auto result = std::max(start, other.start) <= std::min(end, other.end);
#ifdef NEBULA_OPT_DEBUG_VERBOSE
            std::cout
                << std::format("Comparing ranges [{}, {}] and [{}, {}] => {}",
                               start, end, other.start, other.end,
                               (result ? "Overlap" : "Distinct"))
                << std::endl;
#endif
            return result;
        }

    private:
        void validate() const
        {
            if (start > end)
            {
                throw std::runtime_error(std::format("[Error] Range starting point {} is greater than the end point {}", start, end));
            }
        }
    };

    /**
     * Optimizer created Resource
     * The information contained in the struct is enough to know mappings between original and optimizer resources:
     * the list of IntOptimizerResourceUsagePoint structs contain the required information (user_node_id, user_res_id)
     * to map them to the appropriate OptimizerResource using its "id" variable.
     */
    struct OptimizerResource
    {
        int32_t                                  id;
        std::set<IntOptimizerResourceUsagePoint> usage_points;
        IntResourceInfo                          original_desc;
        ResourceType                             type;

        // Ensure image format compatibility
        vk::Format                               format;
        vk::ImageUsageFlags                      usage_flags;

        Range get_usage_range() const
        {
            return Range(usage_points);
        }

        bool insert_usage_points(const std::set<IntOptimizerResourceUsagePoint>& points)
        {
            // Validation for occupied usage points
            std::vector<IntOptimizerResourceUsagePoint> intersection;
            std::set_intersection(std::begin(usage_points), std::end(usage_points), std::begin(points), std::end(points), std::back_inserter(intersection));

            if (!intersection.empty())
            {
                return false;
            }

            for (const auto& point: points)
            {
                usage_points.insert(point);
            }

            return true;
        }
    };

    struct ResourceOptimizationResult
    {
        std::vector<std::string> messages;
        int32_t non_optimizable_count {0};
        int32_t optimized_resource_count {0};
        int32_t original_resource_count {0};
        std::vector<OptimizerResource> resources;
        std::vector<IntResourceInfo> original_resources;
        Range timeline_range {0, 0};
        std::chrono::microseconds time;
    };

    class ResourceOptimizer
    {
    public:
        ResourceOptimizer(const std::vector<std::shared_ptr<Editor::Node>>& nodes, const std::vector<Editor::Edge>& edges, bool verbose = false)
        : m_nodes(nodes), m_edges(edges), m_verbose_logging(verbose)
        {
        }

        ResourceOptimizationResult run()
        {
            const auto start_time = std::chrono::utc_clock::now();

            const auto R = evaluate_required_resources();
            std::vector<OptimizerResource> opt_resources;

            int32_t non_optimizable_count {0};
            for (const auto& ri : R)
            {
                OptimizerResource resource;
                {
                    resource.id = m_id_sequence++;
                    resource.usage_points = {};
                    resource.original_desc = ri;
                    resource.type = ri.type;
                    resource.format = ri.rd.spec.format;
                    resource.usage_flags = ri.rd.spec.usage_flags;
                };

                auto& usage_points = resource.usage_points;

                usage_points = get_usage_points_for_resource_info(ri);
                Range incoming_range(usage_points);

                // Case: Add non-optimizable resource
                if (!ri.optimizable)
                {
                    opt_resources.push_back(resource);
                    non_optimizable_count++;

                    auto msg = std::format("[Optimizer] New, non-optimizable resource with id {} added of type {}", resource.id, get_resource_type_str(resource.type));
                    m_messages.push_back(msg);

                    continue;
                }

#ifdef NEBULA_OPT_DEBUG_VERBOSE
                std::stringstream points_strstr;
                for (const auto& point : usage_points)
                {
                    points_strstr << std::format(" {},", point.point);
                }
                std::cout << std::format("[Usage Points]{} => Range: [{}, {}]", points_strstr.str(), incoming_range.start, incoming_range.end) << std::endl;
#endif

                // Case: No resources
                if (opt_resources.empty())
                {
                    opt_resources.push_back(resource);

                    auto msg = std::format("[Optimizer] New resource with id {} added of type {}", resource.id, get_resource_type_str(resource.type));
                    m_messages.push_back(msg);

                    continue;
                }

                // Case: Try inserting into existing resource
                bool was_inserted = false;
                for (auto& timeline : opt_resources)
                {
                    Range current_range = timeline.get_usage_range();

                    std::bitset<5> flags;
                    {
                        flags[0] = !current_range.overlaps(incoming_range);
                        flags[1] = ri.type == timeline.type;
                        flags[2] = ri.rd.spec.format == timeline.format;
                        flags[3] = ri.rd.spec.usage_flags == timeline.usage_flags;
                        flags[4] = ri.optimizable;
                    }

                    if (flags.all())
                    {
                        was_inserted = timeline.insert_usage_points(usage_points);

                        if (was_inserted)
                        {
                            auto msg = std::format("[Optimizer] Resource added to {} of type {} with {} usage points.", timeline.id, get_resource_type_str(resource.type), usage_points.size());
                            m_messages.push_back(msg);
                        }

                        break;
                    }
                }

                // Case: Failed to insert, add new resource
                if (!was_inserted)
                {
                    opt_resources.push_back(resource);

                    auto msg = std::format("[Optimizer] New resource added with id {} of type {}", resource.id, get_resource_type_str(resource.type));
                    m_messages.push_back(msg);
                }
            }

            const auto end_time = std::chrono::utc_clock::now();

            ResourceOptimizationResult result {
                .messages = m_messages,
                .non_optimizable_count = non_optimizable_count,
                .optimized_resource_count = static_cast<int32_t>(opt_resources.size()),
                .original_resource_count = static_cast<int32_t>(R.size()),
                .resources = opt_resources,
                .original_resources = R,
                .timeline_range = { 0, static_cast<int32_t>(m_nodes.size() - 1) },
                .time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time),
            };

            return result;
        }

    private:
        static std::set<IntOptimizerResourceUsagePoint> get_usage_points_for_resource_info(const IntResourceInfo& resource_info)
        {
            std::set<IntOptimizerResourceUsagePoint> usage_points;

            IntOptimizerResourceUsagePoint producer_usage_point(resource_info);
            usage_points.insert(producer_usage_point);

            for (auto& user : resource_info.users)
            {
                IntOptimizerResourceUsagePoint consumer_point(user);
                usage_points.insert(consumer_point);
            }

            return usage_points;
        }

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

        bool m_verbose_logging {false};
        std::vector<std::string> m_messages;

        const std::set<ResourceType> m_optimizable_types { ResourceType::eDepthImage, ResourceType::eImage };

        const std::vector<std::shared_ptr<Editor::Node>>& m_nodes;  // Nodes in execution order
        const std::vector<Editor::Edge>& m_edges;                   // Original graph edges
    };
}