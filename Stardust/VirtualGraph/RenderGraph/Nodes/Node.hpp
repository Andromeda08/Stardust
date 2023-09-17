#pragma once

#include <map>
#include <memory>
#include <string>
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>

namespace Nebula::RenderGraph
{
    class Node
    {
    public:
        Node() = default;

        virtual void execute(const vk::CommandBuffer& command_buffer) { /* default: no-op */ }

        virtual void initialize() { /* default: no-op */ }

        virtual bool set_resource(const std::string& key, const std::shared_ptr<Resource>& resource)
        {
            if (!_validate_resource(key, resource))
            {
                return false;
            }

            if (!m_resources.contains(key))
            {
                m_resources.insert({key, resource});
                return true;
            }

            m_resources[key] = resource;
            return true;
        }

        virtual ~Node() = default;

    protected:
        virtual bool _validate_resource(const std::string& key, const std::shared_ptr<Resource>& resource)
        {
            return true;
        }

    protected:
        std::map<std::string, std::shared_ptr<Resource>> m_resources;
    };
}