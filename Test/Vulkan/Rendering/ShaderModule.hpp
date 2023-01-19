#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct ShaderModule
    {
    public:
        ShaderModule(std::string const& source, vk::ShaderStageFlagBits shader_stage, vk::Device const& device);

        vk::PipelineShaderStageCreateInfo stage_info() const;

    private:
        static std::vector<char> read_file(std::string const& file);

        vk::ShaderModule module;
        vk::ShaderStageFlagBits stage;
    };
}