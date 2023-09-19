#pragma once

#include <string>

namespace Nebula::RenderGraph
{
    enum class NodeType
    {
        eAmbientOcclusion,
        eAntiAliasing,
        eDeferredRender,
        eDenoise,
        eGaussianBlur,
        eLightingPass,
        eRayTracing,

        // Special Types
        ePresent,
        eSceneProvider,
        eUnknown,
    };
}