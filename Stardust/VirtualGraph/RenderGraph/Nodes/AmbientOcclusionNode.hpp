#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionMode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionOptions.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionStrategy.hpp>
#include <Vulkan/Context.hpp>

namespace Nebula::RenderGraph
{
    class AmbientOcclusionNode : public Node
    {
    public:
        explicit AmbientOcclusionNode(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

    private:
        AmbientOcclusionOptions m_options;
        std::shared_ptr<AmbientOcclusionStrategy> m_mode;

        const sdvk::Context& m_context;

    public:
        const std::vector<ResourceSpecification>& get_resource_specs() const override
        {
            return s_resource_specs;
        }

        static const std::vector<ResourceSpecification> s_resource_specs;
    };
}