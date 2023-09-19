#pragma once

#include "Node.hpp"
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula::RenderGraph
{
    enum class AntiAliasingMode
    {
        eNone,
        eFXAA,
        eFSR2,
    };

    std::string to_string(AntiAliasingMode mode);
    std::string to_full_string(AntiAliasingMode mode);

    enum class FSRMode
    {
        eQuality,
        eBalanced,
        ePerformance
    };

    std::string to_string(FSRMode mode);
    std::string to_full_string(FSRMode mode);

    class FSR
    {
        static bool check_gpu_requirements(const sdvk::Context& ctx);
    };

    struct AntiAliasingNodeOptions
    {
        AntiAliasingMode mode = AntiAliasingMode::eNone;
        FSRMode fsr_mode = FSRMode::eQuality;
    };

    class AntiAliasingNode
    {
    public:


    private:
        AntiAliasingNodeOptions m_options;
    };
}