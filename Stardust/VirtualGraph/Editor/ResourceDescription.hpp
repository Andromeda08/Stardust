#pragma once

#include <string>
#include <uuid.h>
#include <glm/vec4.hpp>
#include <Utility.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceRole.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceType.hpp>

namespace Nebula::RenderGraph::Editor
{
    struct ResourceDescription
    {
        int32_t      id   = sd::util::gen_id();
        uuids::uuid  uuid = uuids::uuid_system_generator{}();
        std::string  name = "Unknown Resource";
        ResourceRole role = ResourceRole::eUnknown;
        ResourceType type = ResourceType::eUnknown;

        ResourceSpecification spec {};

        bool input_is_connected = false;

        ResourceDescription() = default;

        ResourceDescription(std::string&& n, ResourceRole&& r): name(n), role(r) {}

        ResourceDescription(std::string&& n, ResourceRole&& r, ResourceType&& t): name(n), role(r), type(t) {}

        ResourceDescription(const std::string& n, const ResourceRole& r, const ResourceType& t): name(n), role(r), type(t) {}

        std::string role_str() const {
            return get_resource_role_str(role);
        }

        glm::ivec4 pin_color() const
        {
            return get_resource_type_color(type);
        }
    };
}