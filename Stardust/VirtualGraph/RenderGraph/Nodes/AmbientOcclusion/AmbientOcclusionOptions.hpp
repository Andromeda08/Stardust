#pragma once

#include <cstdint>
#include "AmbientOcclusionMode.hpp"

namespace Nebula::RenderGraph
{
    struct AmbientOcclusionOptions
    {
        AmbientOcclusionMode mode {AmbientOcclusionMode::eRTAO};

        // Common
        int32_t    samples {64};
        float      radius {0.5f};
        // SSAO
        float      bias {0.025f};
        // RTAO
        float      power {2.0f};
        int32_t    max_samples {50000};
        vk::Bool32 accumulation_enabled {0};
    };
}