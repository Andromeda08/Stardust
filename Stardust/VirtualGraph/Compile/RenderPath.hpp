#pragma once

#include <memory>
#include <vector>
#include <Nebula/Image.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>

namespace vk
{
    class CommandBuffer;
}

namespace Nebula::RenderGraph
{
    struct RenderPath
    {
        // Nodes in execution order
        std::vector<std::shared_ptr<Node>> nodes;

        // Image resources
        std::map<std::string, std::shared_ptr<Resource>> resources;

        void execute(const vk::CommandBuffer& command_buffer)
        {
            if (!m_is_initialized)
            {
                for (const auto& node : nodes)
                {
                    node->initialize();
                }
            }

            for (const auto& node : nodes)
            {
                node->execute(command_buffer);
            }
        }

    private:
        bool m_is_initialized = false;
    };
}