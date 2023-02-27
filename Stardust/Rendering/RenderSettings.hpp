#pragma once

#include <Rendering/AmbientOcclusion.hpp>

namespace sd
{
    enum class ShadowMode
    {
        eNone,
        eRayQuery
    };

    struct RenderSettings
    {
        struct AO {
            AmbientOcclusionMode mode { AmbientOcclusionMode::eNone };
            bool is_enabled() const { return mode != AmbientOcclusionMode::eNone; }
        } ambient_occlusion;

        struct Shadows {
            ShadowMode mode { ShadowMode::eRayQuery };
            bool is_enabled() const { return mode != ShadowMode::eNone; }
        } shadows;
    };
}