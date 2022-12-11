#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Device.hpp>

namespace re
{
    enum class ShaderStage
    {
        eVertex,
        eFragment,
        eRaygen,
        eClosestHit,
        eMiss,
        eCompute
    };
    static vk::ShaderStageFlagBits to_vk_shader_stage(ShaderStage stage);

    class Shader
    {
    public:
        Shader(const std::string& shader_src, vk::ShaderStageFlagBits stage, const Device& device);

        vk::PipelineShaderStageCreateInfo stage_info() const;

        vk::ShaderModule        _shader_module;
        vk::ShaderStageFlagBits _shader_stage;

    private:
        static std::vector<char> read_file(const std::string& file_name);
    };
}
