#include "OptimizedCompileStrategy.hpp"
#include <chrono>
#include <sstream>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>
#include <VirtualGraph/Compile/Algorithm/Bfs.hpp>
#include <VirtualGraph/Compile/Algorithm/ResourceOptimizer.hpp>
#include <VirtualGraph/Compile/Algorithm/TopologicalSort.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>

namespace Nebula::RenderGraph::Compiler
{
    CompileResult OptimizedCompileStrategy::compile(const std::vector<std::shared_ptr<Editor::Node>>& nodes,
                                                    const std::vector<Editor::Edge>& edges,
                                                    bool verbose)
    {
        CompileResult compile_result = {};
        auto start_time = std::chrono::utc_clock::now();

        // 1. Find unreachable nodes (BFS Traversal)
        auto connected_nodes = filter_unreachable_nodes(nodes);

        // 2. To determine execution order of nodes run Topological Sort based on Logical Nodes and Connections.
        auto execution_order = get_execution_order(connected_nodes);

        // 3. Evaluate and optimize resources
        auto optimizer = std::make_unique<Algorithm::ResourceOptimizer>(execution_order, edges);
        auto optimization_result = optimizer->run();

        // 4. Create resources
        std::map<std::string, std::shared_ptr<Resource>> created_resources; // optimizer_id -> resource
        for (const auto& opt_resource : optimization_result.resources)
        {
            const auto resource_name = std::format("({:%Y-%m-%d %H:%M}) OptGenResource-{}", start_time, opt_resource.id);
            std::shared_ptr<Resource> new_resource;

            if (opt_resource.type == ResourceType::eCamera)
            {
                const auto& camera = m_context.scene()->camera();
                new_resource = std::make_shared<CameraResource>(camera, resource_name);
            }
            else if (opt_resource.type == ResourceType::eObjects)
            {
                const auto& objects = m_context.scene()->objects();
                new_resource = std::make_shared<ObjectsResource>(objects, resource_name);
            }
            else if (opt_resource.type == ResourceType::eTlas)
            {
                const auto& tlas = m_context.scene()->acceleration_structure();
                new_resource = std::make_shared<TlasResource>(tlas, resource_name);
            }
            else if (opt_resource.type == ResourceType::eImage)
            {
                auto image = std::make_shared<Nebula::Image>(m_context.context(),
                                                             opt_resource.format,
                                                             m_context.render_resolution(),
                                                             vk::SampleCountFlagBits::e1,
                                                             opt_resource.usage_flags | vk::ImageUsageFlagBits::eColorAttachment,
                                                             vk::ImageAspectFlagBits::eColor,
                                                             vk::ImageTiling::eOptimal,
                                                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                             resource_name);
                new_resource = std::make_shared<ImageResource>(image, resource_name);
            }
            else if (opt_resource.type == ResourceType::eDepthImage)
            {
                auto image = Nebula::Image::make_depth_image(m_context.render_resolution(),
                                                             m_context.context(),
                                                             resource_name);
                new_resource = std::make_shared<DepthImageResource>(image, resource_name);
            }
            else
            {
                continue;
            }

            auto res_created_msg = std::format("[Compiler] Created {}resource of type \"{}\" with name {}.",
                                               (opt_resource.type == ResourceType::eImage || opt_resource.type == ResourceType::eDepthImage) ? "GPU " : "",
                                               get_resource_type_str(opt_resource.type),
                                               resource_name);
            std::cout << res_created_msg << std::endl;

            created_resources.insert({ std::to_string(opt_resource.id), new_resource });
        }

        // 5. Create nodes
        std::vector<std::shared_ptr<RenderGraph::Node>> created_nodes;
        std::map<int32_t, int32_t> node_mappings; // graph_id -> real_id
        for (const auto& node : execution_order)
        {
            auto n = m_node_factory->create(node, node->type());
            if (n != nullptr)
            {
                created_nodes.push_back(n);
                node_mappings.insert({node->id(), created_nodes.size() - 1 });
            }
        }

        // 6. Connect resources to nodes
        for (const auto& opt_resource : optimization_result.resources)
        {
            // 6.0 Get resource
            auto& resource = created_resources[std::to_string(opt_resource.id)];

            // 6.1 Connect to origin node
            auto& origin = opt_resource.original_desc;
            auto& cnode_origin = created_nodes[node_mappings[origin.origin_node_id]];
            cnode_origin->set_resource(origin.origin_res_name, resource);

            // 6.2 Connect to consumer nodes
            for (const auto& consumer : opt_resource.usage_point_meta)
            {
                auto& cnode_consumer = created_nodes[node_mappings[consumer.user_node_id]];
                cnode_consumer->set_resource(consumer.used_as, resource);
            }
        }

        // 7. Create RenderPath
        auto render_path = std::make_shared<RenderPath>();
        render_path->resources = created_resources;
        render_path->nodes = created_nodes;

        // 8. Finish up & Create compile result
        auto end_time = std::chrono::utc_clock::now();
        auto compile_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        compile_result.compile_time = compile_time;
        compile_result.failure_message = "";
        compile_result.logs = logs;
        compile_result.render_path = render_path;
        compile_result.success = true;

        // 9. Write logs
        // write_logs_to_file(std::format("GraphCompile_Optimized_Log_{:%Y-%m-%d_%H-%M}_{}", start_time, compile_result.success ? "Success" : "Failed"));

        // 10. If verbose mode: write graph dump
        if (verbose)
        {
            write_graph_state_dump(*render_path, std::format("GraphCompile_Optimized_Log_{:%Y-%m-%d_%H-%M}_Dump", start_time));
        }

        return compile_result;
    }

    std::vector<std::shared_ptr<Editor::Node>>
    OptimizedCompileStrategy::filter_unreachable_nodes(const std::vector<std::shared_ptr<Editor::Node>>& nodes)
    {
        std::vector<std::shared_ptr<Editor::Node>> result;

        std::shared_ptr<Editor::Node> root_node;
        for (const auto& node : nodes)
        {
            if (node->type() == NodeType::eSceneProvider)
            {
                root_node = node;
            }
        }

        if (!root_node)
        {
            throw std::runtime_error("[Error] Graph must contain a SceneProvider node");
        }

        auto bfs = std::make_unique<Algorithm::Bfs>(nodes);
        auto reachable_node_ids = bfs->execute(root_node);
        for (const auto& node : nodes)
        {
            if (reachable_node_ids.contains(node->id()))
            {
                result.push_back(node);
            }
        }

        return result;
    }

    std::vector<std::shared_ptr<Editor::Node>>
    OptimizedCompileStrategy::get_execution_order(const std::vector<std::shared_ptr<Editor::Node>>& nodes)
    {
        std::vector<std::shared_ptr<Editor::Node>> result;

        auto tsort = std::make_unique<Algorithm::TopologicalSort>(nodes);
        try
        {
            result = tsort->execute();
        }
        catch (const std::runtime_error& ex)
        {
            throw ex;
        }

        return result;
    }
}