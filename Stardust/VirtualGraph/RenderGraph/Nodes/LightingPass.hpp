#pragma once

#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include "Node.hpp"
#include "AntiAliasingNode.hpp"

namespace Nebula::RenderGraph
{
    enum class LightingPassShadowMode
    {
        eRayQuery,
        eShadowMaps,
        eNone,
    };

    std::string to_string(LightingPassShadowMode shadow_mode);

    struct LightingPassOptions
    {
        bool include_ao {false};
        bool include_aa {false};
        LightingPassShadowMode shadow_mode {LightingPassShadowMode::eNone};
    };

    class LightingPass : public Node
    {
    public:
        static const std::vector<ResourceSpecification> s_resource_specs;

        const std::vector<ResourceSpecification>& get_resource_specs() const override { return s_resource_specs; }
    };
}