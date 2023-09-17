#pragma once

namespace Nebula
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

        // TODO: Combine and Render should be deprecated after implementing proper deferred shading
        eCombine,
        eRender,
    };
}