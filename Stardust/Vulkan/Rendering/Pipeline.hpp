#pragma once

#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct Pipeline
    {
        vk::PipelineLayout pipeline_layout;
        vk::Pipeline       pipeline;
    };
}