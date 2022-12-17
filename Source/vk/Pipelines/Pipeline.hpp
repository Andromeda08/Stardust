#pragma once

#include <vulkan/vulkan.hpp>

struct Pipeline
{
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline       pipeline;
};