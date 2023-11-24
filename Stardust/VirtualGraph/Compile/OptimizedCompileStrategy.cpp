#include "OptimizedCompileStrategy.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <VirtualGraph/Editor/Edge.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/Editor/ResourceDescription.hpp>
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
        logs.push_back(std::format("[Compiler] Compiling started at {:%Y-%m-%d %H:%M}", start_time));

        if (verbose)
        {
            std::stringstream input_nodes;
            input_nodes << "[Info] Received nodes as input:";
            for (const auto& node : nodes)
            {
                input_nodes << std::format(" [{}]", node->name());
            }
            logs.push_back(input_nodes.str());
        }

        // 1. Find unreachable nodes (BFS Traversal)
        std::vector<std::shared_ptr<Editor::Node>> connected_nodes;
        try
        {
            connected_nodes = filter_unreachable_nodes(nodes);
        }
        catch (const std::runtime_error& ex)
        {
            return make_failed_result(ex.what());
        }

        if (verbose)
        {
            logs.push_back(std::format("[Compiler] Found and culled {} unreachable node(s)", std::to_string(nodes.size() - connected_nodes.size())));
        }

        // 2. To determine execution order of nodes run Topological Sort based on Logical Nodes and Connections.
        std::vector<std::shared_ptr<Editor::Node>> execution_order;
        try
        {
            execution_order = get_execution_order(connected_nodes);
        }
        catch (const std::runtime_error& ex)
        {
            return make_failed_result(ex.what());
        }


        if (verbose)
        {
            std::stringstream input_nodes;
            input_nodes << "[Compiler] Node execution order:";
            for (const auto& node : execution_order)
            {
                input_nodes << std::format(" [{}]", node->name());
            }
            logs.push_back(input_nodes.str());
        }

        // 3. Evaluate and optimize resources
        auto optimizer = std::make_unique<Algorithm::ResourceOptimizer>(execution_order, edges, verbose);
        Algorithm::ResourceOptimizationResult optimization_result;
        try
        {
            optimization_result = optimizer->run();
            for (const auto& msg : optimization_result.messages)
            {
                logs.push_back(msg);
            }
        }
        catch (const std::runtime_error& ex)
        {
            return make_failed_result(ex.what());
        }

        logs.push_back(std::format("[Compiler] Resource optimization finished in {} microseconds", optimization_result.time.count()));

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
            else if (opt_resource.type == ResourceType::eBuffer)
            {
                const auto& buffer = m_context.scene()->object_descriptions();
                new_resource = std::make_shared<BufferResource>(buffer, resource_name);
            }
            else if (opt_resource.type == ResourceType::eScene)
            {
                new_resource = std::make_shared<SceneResource>(m_context.scene(), resource_name);
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
            logs.push_back(res_created_msg);

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
            for (const auto& consumer : opt_resource.usage_points)
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

        logs.push_back(std::format("[Compiler] Graph compiled in {} ms", compile_time.count()));

        compile_result.compile_time = compile_time;
        compile_result.logs = logs;
        compile_result.render_path = render_path;
        compile_result.success = true;

        // 9. Write logs
        // write_logs_to_file(std::format("GraphCompile_Optimized_Log_{:%Y-%m-%d_%H-%M}_{}", start_time, compile_result.success ? "Success" : "Failed"));

        // 10. Write optimization result dump
        if (verbose)
        {
            write_optimization_results(optimization_result, std::format("OptimizerResult_{:%Y-%m-%d_%H-%M}_Dump", start_time));
        }

        // 11. If verbose mode: write graph dump
        if (verbose)
        {
            write_graph_state_dump(*render_path, std::format("GraphCompile_Optimized_Log_{:%Y-%m-%d_%H-%M}_Dump", start_time));
        }

        return compile_result;
    }

    void OptimizedCompileStrategy::write_optimization_results(const Algorithm::ResourceOptimizationResult& optres, const std::string& file_name)
    {
        std::vector<std::string> dump;

        dump.push_back(file_name);
        dump.emplace_back("Optimization summary:");
        dump.push_back(std::format("\tOriginal resource count: {}", optres.original_resource_count));
        dump.push_back(std::format("\tFrom which can be optimized: {}", std::to_string(optres.original_resource_count - optres.non_optimizable_count)));
        dump.push_back(std::format("\tResource count post-optimization: {}", optres.optimized_resource_count));
        dump.push_back(std::format("\tReduction: {}", std::to_string(optres.original_resource_count - optres.optimized_resource_count)));
        dump.emplace_back("========== Unoptimized resource timeline ==========");
        int32_t res_num {0};
        for (const auto& res : optres.original_resources)
        {
            std::stringstream sstr;
            sstr << std::format("[{} | {}]\t|", res.origin_res_name, get_resource_type_str(res.type));

            for (int32_t i = optres.timeline_range.start; i < optres.timeline_range.end + 1; i++)
            {
                auto user = std::find_if(std::begin(res.users), std::end(res.users), [&](const auto& it){
                   return it.node_idx == i;
                });

                if (user == std::end(res.users))
                {
                    sstr << ((res.origin_node_idx == i) ? " x " : " - ");
                }
                else
                {
                    sstr << " x ";
                }
            }

            sstr << "|";
            dump.push_back(sstr.str());
            res_num++;
        }

        dump.emplace_back("========== Optimized resource timeline ==========");
        for (const auto& res : optres.resources)
        {
            std::stringstream sstr;
            sstr << std::format("[Resource {} | {}]\t|", res.id, get_resource_type_str(res.type));

            for (int32_t i = optres.timeline_range.start; i < optres.timeline_range.end + 1; i++)
            {
                auto user = std::find_if(std::begin(res.usage_points), std::end(res.usage_points), [&](const auto& it){
                    return it.point == i;
                });

                if (user == std::end(res.usage_points))
                {
                    sstr << " - ";
                }
                else
                {
                    sstr << " x ";
                }
            }
            sstr << "|";

            for (const auto& user : res.usage_points)
            {
                sstr << std::format(" {} ", user.used_as);
            }

            dump.push_back(sstr.str());
        }
        dump.emplace_back("(- = not used, x = used)");

        auto path = std::format("logs/{}.txt", file_name);
        std::fstream fs(path);
        fs.open(path, std::ios_base::out);
        std::ostream_iterator<std::string> os_it(fs, "\n");
        std::copy(dump.begin(), dump.end(), os_it);
        fs.close();
    }
}