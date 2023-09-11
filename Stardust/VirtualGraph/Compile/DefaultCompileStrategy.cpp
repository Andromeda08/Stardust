#include "DefaultCompileStrategy.hpp"

#include <chrono>
#include <set>
#include <sstream>
#include <Benchmarking.hpp>
#include <Nebula/Image.hpp>
#include <VirtualGraph/Node.hpp>
#include <VirtualGraph/GraphEditor.hpp>
#include <VirtualGraph/ResourceDescription.hpp>
#include <VirtualGraph/Compile/NodeFactory.hpp>
#include <VirtualGraph/Compile/Algorithm/Bfs.hpp>
#include <VirtualGraph/Compile/Algorithm/TopologicalSort.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>

namespace Nebula::Editor
{
    CompileResult DefaultCompileStrategy::compile(const std::vector<std::shared_ptr<Node>>& nodes, bool verbose)
    {
        CompileResult result = {};

        auto begin_time = std::chrono::utc_clock::now();
        logs.push_back(std::format("[Info] Compiling started at {:%Y-%m-%d %H:%M}", begin_time));

        if (verbose)
        {
            std::stringstream input_nodes;
            input_nodes << "[Verbose] Received nodes as input:";
            for (const auto& node : nodes)
            {
                input_nodes << std::format(" [{}]", node->name());
            }
            logs.push_back(input_nodes.str());
        }

        // 1. Find unreachable nodes (BFS Traversal)
        #pragma region Filter out unreachable nodes by BFS Traversal of RG
        std::chrono::milliseconds filter_time;
        std::vector<std::shared_ptr<Node>> connected_nodes;

        std::shared_ptr<Node> root_node;
        for (const auto& node : nodes)
        {
            if (node->type() == NodeType::eSceneProvider)
            {
                root_node = node;
            }
        }
        if (root_node == nullptr)
        {
            result.success = false;
            logs.emplace_back("[Error] Graph must contain a SceneProvider node.");
            result.failure_message = logs.back();
            write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H:%M}_{}", begin_time, result.success ? "Success" : "Failed"));
            return result;
        }

        filter_time = sd::bm::measure([&](){
            auto bfs = std::make_unique<Bfs>(nodes);
            auto reachable_node_ids = bfs->execute(root_node);
            for (const auto& node : nodes)
            {
                if (reachable_node_ids.contains(node->id()))
                {
                    connected_nodes.push_back(node);
                }
            }
        });
        logs.push_back(std::format("[Info] Found {} unreachable node(s). ({}ms)",
                                   std::to_string(nodes.size() - connected_nodes.size()),
                                   filter_time.count()));
        #pragma endregion

        // 2. To determine execution order of nodes run Topological Sort based on Logical Nodes and Connections.
        #pragma region Topological Sort on reachable nodes
        std::chrono::milliseconds tsort_time;
        auto topological_sort = std::make_unique<TopologicalSort>(connected_nodes);
        std::vector<std::shared_ptr<Node>> topological_ordering;
        try
        {
            tsort_time = sd::bm::measure<std::chrono::milliseconds>([&](){
                topological_ordering = topological_sort->execute();
            });
        }
        catch (const std::runtime_error& ex)
        {
            result.success = false;
            result.logs.emplace_back(ex.what());
            result.failure_message = logs.back();
            write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H:%M}_{}", begin_time, result.success ? "Success" : "Failed"));
            return result;
        }

        std::stringstream tsort_log;
        tsort_log << std::format("[Info] Generated topological ordering ({}ms):", tsort_time.count());
        for (const auto& node : topological_ordering)
        {
            tsort_log << std::format(" [{}]", node->name());
        }
        logs.push_back(tsort_log.str());
        #pragma endregion

        // 3. Evaluate required resources
        #pragma region Evaluate required resources
        std::chrono::milliseconds resource_eval_time;
        std::map<int32_t, ResourceDescription> required_resources;
        resource_eval_time = sd::bm::measure<std::chrono::milliseconds>([&]() {
            for (const auto& node: topological_ordering) {
                const auto& resources = node->resources();
                for (const auto& resource: resources) {
                    if (required_resources.contains(resource.id))
                    {
                        continue;
                    }
                    if (resource.role == ResourceRole::eInput)
                    {
                        continue;
                    }

                    required_resources.insert({resource.id, resource});
                }
            }
        });
        logs.push_back(std::format("[Info] Required resource count: {} ({}ms)", std::to_string(required_resources.size()), resource_eval_time.count()));
        #pragma endregion

        // 4. Create GPU resources
        #pragma region Create GPU resources
        std::chrono::milliseconds create_time;
        std::map<std::string, std::shared_ptr<Nebula::Image>> images;
        create_time = sd::bm::measure<std::chrono::milliseconds>([&](){
            for (const auto& [id, resource] : required_resources)
            {
                if (resource.type != ResourceType::eImage)
                {
                    continue;
                }

                const auto resource_name = std::format("({:%Y-%m-%d %H:%M}) {}-{}", begin_time, resource.name, id);

                const auto& res_spec = resource.spec;
                auto image = std::make_shared<Nebula::Image>(m_context,
                                                             res_spec.format,
                                                             res_spec.extent,
                                                             res_spec.sample_count,
                                                             res_spec.usage_flags,
                                                             res_spec.aspect_flags,
                                                             res_spec.tiling,
                                                             res_spec.memory_flags,
                                                             resource_name);

                images.insert({ resource.name, image });
                if (verbose)
                {
                    logs.push_back(std::format("[Verbose] Created GPU resource: {}", resource_name));
                }
            }
        });
        logs.push_back(std::format("[Info] Created {} resource(s) ({}ms)", std::to_string(images.size()), create_time.count()));
        #pragma endregion

        // 5. Create real nodes
        #pragma region Create real nodes
        auto node_factory = std::make_shared<NodeFactory>(m_context, Editor::GraphEditor::s_selected_scene);
        std::vector<std::shared_ptr<RenderGraph::Node>> real_nodes;
        for (const auto& node : connected_nodes)
        {
            auto n = node_factory->create(node->type());
            if (n != nullptr)
            {
                real_nodes.push_back(n);
            }
        }
        #pragma endregion

        // 6. Connect resources to nodes
        #pragma region Connect resources to nodes
        for (const auto& node : real_nodes)
        {
            for (const auto& [name, img] : images)
            {
                auto res = std::make_shared<ImageResource>(img);
                node->set_resource(name, res);
            }
        }
        #pragma endregion

        // 7. Finalize, pack result into a RenderPath and do final statistics
        #pragma region Finalize
        auto compile_time = filter_time + tsort_time + resource_eval_time + create_time;
        logs.push_back(std::format("[Info] Graph compiled in {} ms", compile_time.count()));

        RenderGraph::RenderPath render_path;
        render_path.resources = images;
        render_path.nodes = real_nodes;

        result.compile_time = compile_time;
        result.logs = logs;
        result.success = true;
        result.render_path = render_path;
        #pragma endregion

        // 8. Write logs to file
        write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H-%M}_{}", begin_time, result.success ? "Success" : "Failed"));

        return result;
    }
}