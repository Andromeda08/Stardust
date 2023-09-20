#pragma once

#include <map>
#include <string>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionMode.hpp>
#include <VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionOptions.hpp>

namespace Nebula::RenderGraph
{
    class AmbientOcclusionStrategy
    {
    public:
        struct Factory
        {
            explicit Factory(const sdvk::Context& context,
                             std::map<std::string, std::shared_ptr<Resource>>& resources)
            : m_context(context)
            , m_resources(resources)
            {
            }

            AmbientOcclusionStrategy* create(AmbientOcclusionMode mode);

        private:
            const sdvk::Context& m_context;
            std::map<std::string, std::shared_ptr<Resource>>& m_resources;
        };

    public:
        explicit AmbientOcclusionStrategy(const sdvk::Context& context,
                                          std::map<std::string, std::shared_ptr<Resource>>& resources)
        : m_context(context)
        , m_resources(resources)
        {
        }

        virtual void execute(const vk::CommandBuffer& command_buffer) = 0;

        virtual void initialize(const AmbientOcclusionOptions& options) = 0;

        virtual ~AmbientOcclusionStrategy() = default;

    protected:
        std::map<std::string, std::shared_ptr<Resource>>& m_resources;
        const sdvk::Context& m_context;
    };
}