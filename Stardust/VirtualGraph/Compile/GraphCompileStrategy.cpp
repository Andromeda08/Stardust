#include "GraphCompileStrategy.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <format>
#include <string>
#include <VirtualGraph/Common/NodeType.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>

namespace Nebula::RenderGraph::Compiler
{
    std::string to_string(ResourceType type)
    {
        switch (type)
        {
            case RenderGraph::ResourceType::eBuffer:
                return "Buffer";
            case RenderGraph::ResourceType::eCamera:
                return "Camera";
            case RenderGraph::ResourceType::eDepthImage:
                return "Depth Image";
            case RenderGraph::ResourceType::eImage:
                return "Image";
            case RenderGraph::ResourceType::eImageArray:
                return "Image Array";
            case RenderGraph::ResourceType::eObjects:
                return "Objects";
            case RenderGraph::ResourceType::eTlas:
                return "Tlas";
            case RenderGraph::ResourceType::eUnknown:
                // Falls through
            default:
                return "Unknown";
        }
    }

    std::string to_string(NodeType type)
    {
        switch (type)
        {
            case NodeType::eAmbientOcclusion:
                return "Ambient Occlusion";
            case NodeType::eAntiAliasing:
                return "Anti-Aliasing";
            case NodeType::eDeferredRender:
                return "Deferred Pass";
            case NodeType::eDenoise:
                return "Denoise";
            case NodeType::eGaussianBlur:
                return "Blur";
            case NodeType::eLightingPass:
                return "Lighting Pass";
            case NodeType::eRayTracing:
                return "Ray Tracing";
            case NodeType::ePresent:
                return "Present";
            case NodeType::eSceneProvider:
                return "Scene Provider";
            case NodeType::eUnknown:
                // Falls Through
            default:
                return "Unknown";
        }
    }

    void GraphCompileStrategy::write_logs_to_file(const std::string& file_name)
    {
        auto path = std::format("logs/{}.txt", file_name);
        std::fstream fs(path);
        fs.open(path, std::ios_base::out);
        std::ostream_iterator<std::string> os_it(fs, "\n");
        std::copy(logs.begin(), logs.end(), os_it);
        fs.close();
    }

    void GraphCompileStrategy::write_graph_state_dump(const RenderPath& render_path, const std::string& file_name)
    {
        std::vector<std::string> dump;
        dump.emplace_back("[=====[ Begin Resources ]=====]");
        for (const auto& res : render_path.resources)
        {
            dump.push_back(std::format("[Resource] {}", res.second->name()));
            dump.push_back(std::format("\tType: {}", to_string(res.second->type())));
            dump.push_back(std::format("\tValid: {}", res.second->is_valid() ? "true" : "false"));
        }
        dump.emplace_back("[=====[ End Resources ]=====]");

        dump.emplace_back(" ");

        dump.emplace_back("[=====[ Begin Nodes ]=====]");
        for (const auto& node : render_path.nodes)
        {
            dump.push_back(std::format("[Node] {}", node->name()));
            dump.push_back(std::format("\tType: {}", to_string(node->type())));
            dump.emplace_back("\tResources:");

            auto& resources = node->resources();
            for (const ResourceSpecification& rspec : node->get_resource_specs())
            {
                const bool is_connected = resources.contains(rspec.name);
                const std::string in_or_out = (rspec.role == ResourceRole::eInput) ? "Input" : "Output";
                dump.push_back(
                    is_connected
                    ? std::format("\t\t{}: Connected to: {} as {}", rspec.name, resources[rspec.name]->name(), in_or_out)
                    : std::format("\t\t{}: Missing", rspec.name)
                );
            }
        }
        dump.emplace_back("[=====[End Nodes]=====]");

        auto path = std::format("logs/{}.txt", file_name);
        std::fstream fs(path);
        fs.open(path, std::ios_base::out);
        std::ostream_iterator<std::string> os_it(fs, "\n");
        std::copy(dump.begin(), dump.end(), os_it);
        fs.close();
    }
}
