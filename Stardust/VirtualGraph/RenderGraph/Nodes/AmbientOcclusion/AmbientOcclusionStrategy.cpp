#include "AmbientOcclusionStrategy.hpp"
#include <stdexcept>
#include "RayTracedAO.hpp"
#include "ScreenSpaceAO.hpp"

namespace Nebula::RenderGraph
{

    AmbientOcclusionStrategy* AmbientOcclusionStrategy::Factory::create(AmbientOcclusionMode mode)
    {
        switch (mode)
        {
            case AmbientOcclusionMode::eSSAO:
                return new ScreenSpaceAO(m_context, m_resources);
            case AmbientOcclusionMode::eRTAO:
                return new RayTracedAO(m_context, m_resources);
            case AmbientOcclusionMode::eUnknown:
                // Falls through
            default:
                throw std::runtime_error("[Error] Unknown ambient occlusion mode!");
        }
    }
}