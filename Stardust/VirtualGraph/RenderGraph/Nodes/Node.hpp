#pragma once

#include <map>
#include <memory>
#include <string>
#include <VirtualGraph/RenderGraph/Resources/Resource.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/Common/NodeType.hpp>

#define DEF_RESOURCE_REQUIREMENTS()                                                                            \
public:                                                                                                        \
    const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; } \
    static const std::vector<ResourceSpecification> s_resource_specs;

namespace Nebula::RenderGraph
{
    class Node
    {
    public:
        Node() = default;

        Node(std::string name, NodeType type)
        : m_name(name)
        , m_type(type)
        {
        }

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

        std::map<std::string, std::shared_ptr<Resource>>& resources() { return m_resources; }

        virtual const std::vector<ResourceSpecification>& get_resource_specs() const = 0;

        const std::string& name() const
        {
            return m_name;
        }

        NodeType type() const
        {
            return m_type;
        }

    protected:
        virtual bool _validate_resource(const std::string& key, const std::shared_ptr<Resource>& resource)
        {
            return true;
        }

    protected:
        std::map<std::string, std::shared_ptr<Resource>> m_resources;

    private:
        const std::string m_name = "Unknown Node";
        const NodeType    m_type = NodeType::eUnknown;
    };
}