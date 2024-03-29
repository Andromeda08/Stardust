#include "DefaultCompileStrategy.hpp"
#include <cstdint>
#include <chrono>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <Benchmarking.hpp>
#include <Nebula/Image.hpp>
#include <VirtualGraph/Common/ResourceType.hpp>
#include <VirtualGraph/Compile/Algorithm/Bfs.hpp>
#include <VirtualGraph/Compile/Algorithm/TopologicalSort.hpp>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>

namespace Nebula::RenderGraph::Compiler
{
    CompileResult DefaultCompileStrategy::compile(const std::vector<std::shared_ptr<Editor::Node>>& nodes,
                                                  const std::vector<Editor::Edge>& edges,
                                                  bool verbose)
    {
        std::cout
            << "// ----------------------------------------------------------------------\n"
            << "// You are using the legacy compiler strategy with no optimizations,\n"
            << "// consider using the optimized compiler instead.\n"
            << "// ----------------------------------------------------------------------"
            << std::endl;

        CompileResult result = {};

        auto begin_time = std::chrono::utc_clock::now();
        m_logs.push_back(std::format("[Info] Compiling started at {:%Y-%m-%d %H:%M}", begin_time));

        if (verbose)
        {
            std::stringstream input_nodes;
            input_nodes << "[Verbose] Received nodes as input:";
            for (const auto& node : nodes)
            {
                input_nodes << std::format(" [{}]", node->name());
            }
            m_logs.push_back(input_nodes.str());
        }

        // 1. Find unreachable nodes (BFS Traversal)
        #pragma region Filter out unreachable nodes by BFS Traversal of RenderGraph

        std::chrono::milliseconds filter_time;
        std::vector<std::shared_ptr<Editor::Node>> connected_nodes;

        std::shared_ptr<Editor::Node> root_node;
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
            m_logs.emplace_back("[Error] Graph must contain a SceneProvider node.");
            result.failure_message = m_logs.back();
            write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H:%M}_{}", begin_time, result.success ? "Success" : "Failed"));
            return result;
        }

        filter_time = sd::bm::measure([&](){
            const auto bfs = std::make_unique<Algorithm::Bfs>(nodes);
            const auto reachable_node_ids = bfs->execute(root_node);
            for (const auto& node : nodes)
            {
                if (reachable_node_ids.contains(node->id()))
                {
                    connected_nodes.push_back(node);
                }
            }
        });
        m_logs.push_back(std::format("[Info] Found {} unreachable node(s). ({}ms)",
                                   std::to_string(nodes.size() - connected_nodes.size()),
                                   filter_time.count()));

        #pragma endregion

        // 2. To determine execution order of nodes run Topological Sort based on Logical Nodes and Connections.
        #pragma region Topological Sort on reachable nodes

        std::chrono::milliseconds tsort_time;
        auto topological_sort = std::make_unique<Algorithm::TopologicalSort>(connected_nodes);
        std::vector<std::shared_ptr<Editor::Node>> topological_ordering;
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
            result.failure_message = m_logs.back();
            write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H:%M}_{}", begin_time, result.success ? "Success" : "Failed"));
            return result;
        }

        std::stringstream tsort_log;
        tsort_log << std::format("[Info] Generated topological ordering ({}ms):", tsort_time.count());
        for (const auto& node : topological_ordering)
        {
            tsort_log << std::format(" [{}]", node->name());
        }
        m_logs.push_back(tsort_log.str());

        #pragma endregion

        // 3. Evaluate required resources
        #pragma region Evaluate required resources

        std::chrono::milliseconds resource_eval_time;
        std::map<int32_t, Editor::ResourceDescription> required_resources;
        resource_eval_time = sd::bm::measure<std::chrono::milliseconds>([&] {
            for (const auto& node: topological_ordering)
            {
                for (const auto& resources = node->resources();
                     const auto& resource: resources)
                {
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
        m_logs.push_back(std::format("[Info] Required resource count: {} ({}ms)", std::to_string(required_resources.size()), resource_eval_time.count()));

        #pragma endregion

        // 4. Create resources
        #pragma region Create resources

        std::chrono::milliseconds create_time;
        std::map<std::string, std::shared_ptr<Resource>> created_resources;
        std::set gpu_types = { ResourceType::eImage, ResourceType::eDepthImage };

        create_time = sd::bm::measure<std::chrono::milliseconds>([&]{
            for (const auto& [id, resource] : required_resources)
            {
                if (resource.role == ResourceRole::eInput)
                {
                    continue;
                }

                const auto resource_name = std::format("({:%Y-%m-%d %H:%M}) {}-{}", begin_time, resource.name, id);
                const auto& res_spec = resource.spec;
                std::shared_ptr<Resource> new_res;

                if (gpu_types.contains(resource.type))
                {
                    if (resource.type == ResourceType::eImage)
                    {
                        auto image = std::make_shared<Image>(m_context.context(), res_spec.format, m_context.render_resolution(),
                                                             res_spec.sample_count, res_spec.usage_flags | vk::ImageUsageFlagBits::eColorAttachment,
                                                             res_spec.aspect_flags, res_spec.tiling, res_spec.memory_flags,
                                                             resource_name);
                        new_res = std::make_shared<ImageResource>(image, resource_name);
                    }
                    else if (resource.type == ResourceType::eDepthImage)
                    {
                        auto image = Image::make_depth_image(m_context.render_resolution(), m_context.context(), resource_name);
                        new_res = std::make_shared<DepthImageResource>(image, resource_name);
                    }
                    else
                    {
                        continue;
                    }

                    if (verbose)
                    {
                        m_logs.push_back(std::format("[Verbose] Created GPU resource: {}", resource_name));
                    }
                }
                else
                {
                    if (resource.type == ResourceType::eCamera)
                    {
                        const auto& camera = m_context.scene()->camera();
                        new_res = std::make_shared<CameraResource>(camera, resource_name);
                    }
                    else if (resource.type == ResourceType::eObjects)
                    {
                        const auto& objects = m_context.scene()->objects();
                        new_res = std::make_shared<ObjectsResource>(objects, resource_name);
                    }
                    else if (resource.type == ResourceType::eTlas)
                    {
                        const auto& tlas = m_context.scene()->acceleration_structure();
                        new_res = std::make_shared<TlasResource>(tlas, resource_name);
                    }
                    else if (resource.type == ResourceType::eBuffer)
                    {
                        const auto& buffer = m_context.scene()->object_descriptions();
                        new_res = std::make_shared<BufferResource>(buffer, resource_name);
                    }
                    else if (resource.type == ResourceType::eScene)
                    {
                        new_res = std::make_shared<SceneResource>(m_context.scene(), resource_name);
                    }
                    else
                    {
                        continue;
                    }

                    if (verbose)
                    {
                        m_logs.push_back(std::format("[Verbose] Created {} resource: {}", get_resource_type_str(resource.type), resource_name));
                    }
                }

                created_resources.insert({resource.name, new_res });
            }
        });
        m_logs.push_back(std::format("[Info] Created {} resource(s) ({}ms)", std::to_string(created_resources.size()), create_time.count()));

        #pragma endregion

        // 5. Create real nodes
        #pragma region Create real nodes

        std::vector<std::shared_ptr<Node>> real_nodes;
        std::map<int32_t, int32_t> id_to_node;
        auto node_creation_time = sd::bm::measure<std::chrono::milliseconds>([&](){
            for (const auto& node : topological_ordering)
            {
                if (const auto n = m_node_factory->create(node, node->type());
                    n != nullptr)
                {
                    real_nodes.push_back(n);
                    id_to_node.insert({ node->id(), real_nodes.size() - 1 });
                }
            }
        });

        #pragma endregion

        // 6. Connect resources to nodes
        #pragma region Connect resources to nodes

        auto resource_connection_time = sd::bm::measure<std::chrono::milliseconds>([&]{
            // Set Outputs
            for (const auto& node : real_nodes)
            {
                for (const auto& [name, res] : created_resources)
                {
                    node->set_resource(name, res);
                }
            }

            // Set Inputs
            for (const auto& edge : edges)
            {
                // Are the nodes still valid
                if (!id_to_node.contains(edge.start.node_id))
                {
                    continue;
                }

                if (!id_to_node.contains(edge.end.node_id))
                {
                    continue;
                }

                // Get resource to be connected
                auto& resource = created_resources[edge.start.res_name];

                // Set resource
                const auto& end_node = real_nodes[id_to_node[edge.end.node_id]];
                end_node->set_resource(edge.end.res_name, resource);
            }
        });

        #pragma endregion

        // 7. Finalize, pack result into a RenderPath and do final statistics
        #pragma region Finalize

        auto compile_time = filter_time + tsort_time + resource_eval_time + create_time + node_creation_time + resource_connection_time;
        m_logs.push_back(std::format("[Info] Graph compiled in {} ms", compile_time.count()));

        auto render_path = std::make_shared<RenderPath>();
        render_path->resources = created_resources;
        render_path->nodes = real_nodes;

        result.compile_time = compile_time;
        result.logs = m_logs;
        result.success = true;
        result.render_path = render_path;

        #pragma endregion

        // 8. Write logs to file
        write_logs_to_file(std::format("GraphCompile_Log_{:%Y-%m-%d_%H-%M}_{}", begin_time, result.success ? "Success" : "Failed"));

        // 9. Dump graph state
        write_graph_state_dump(*render_path, std::format("GraphCompile_Log_{:%Y-%m-%d_%H-%M}_{}", begin_time, "Dump"));

        return result;
    }
}