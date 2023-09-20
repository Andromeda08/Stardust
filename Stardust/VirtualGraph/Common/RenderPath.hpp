#pragma once

#include <memory>
#include <vector>
#include <Nebula/Barrier.hpp>
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
        std::vector<std::shared_ptr<Node>> nodes;

        // Image resources
        std::map<std::string, std::shared_ptr<Resource>> resources;

        void execute(const vk::CommandBuffer& command_buffer)
        {
            if (!m_is_initialized)
            {
                for (const auto& [id, resource] : resources)
                {
                    if (resource->type() == ResourceType::eImage)
                    {
                        auto image = dynamic_cast<ImageResource&>(*resource).get_image();
                        Nebula::Sync::ImageBarrier(image, image->state().layout, vk::ImageLayout::eGeneral).apply(command_buffer);
                    }
                }

                for (const auto& node : nodes)
                {
                    node->initialize();
                }

                m_is_initialized = true;
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