#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include <Vulkan/GraphicsPipeline/ShaderModule.hpp>

class ComputePipeline
{
public:
    static vk::Pipeline make_compute_pipeline(const std::string& shader, vk::PipelineLayout layout, const Device& device)
    {
        ShaderModule sh(vk::ShaderStageFlagBits::eCompute, shader, device);
        vk::ComputePipelineCreateInfo create_info;
        create_info.setLayout(layout);
        create_info.setStage(sh.stage_info());
        vk::Pipeline pipeline;
        auto result = device.handle().createComputePipelines(nullptr, 1, &create_info, nullptr, &pipeline, device.dispatch());
        return pipeline;
    }
};