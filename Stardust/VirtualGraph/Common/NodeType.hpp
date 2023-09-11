#pragma once

namespace Nebula
{
    enum class NodeType
    {
        eSceneProvider,
        eRender,
        eAmbientOcclusion,
        eDenoise,
        eCombine,
        ePresent,
        eRayTracer,
        eUnknown
    };
}