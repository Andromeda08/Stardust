#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#include "Shader.hpp"

#include <fstream>

namespace re
{
    vk::ShaderStageFlagBits to_vk_shader_stage(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::eVertex: return vk::ShaderStageFlagBits::eVertex;
            case ShaderStage::eFragment: return vk::ShaderStageFlagBits::eFragment;
            case ShaderStage::eRaygen: return vk::ShaderStageFlagBits::eRaygenKHR;
            case ShaderStage::eClosestHit: return vk::ShaderStageFlagBits::eClosestHitKHR;
            case ShaderStage::eMiss: return vk::ShaderStageFlagBits::eMissKHR;
            case ShaderStage::eCompute: return vk::ShaderStageFlagBits::eCompute;
            default:
                return vk::ShaderStageFlagBits::eAll;
        }
    }

    Shader::Shader(const std::string &shader_src, vk::ShaderStageFlagBits stage, const Device &device)
    : _shader_stage(stage)
    {
        auto shader_source_code = Shader::read_file(shader_src);

        vk::ShaderModuleCreateInfo create_info;
        create_info.setCodeSize(sizeof(char) * shader_source_code.size());
        create_info.setPCode(reinterpret_cast<const uint32_t*>(shader_source_code.data()));
        auto result = device.handle().createShaderModule(&create_info, nullptr, &_shader_module);
    }

    vk::PipelineShaderStageCreateInfo Shader::stage_info() const
    {
        vk::PipelineShaderStageCreateInfo stage_info;
        stage_info.setStage(_shader_stage);
        stage_info.setModule(_shader_module);
        stage_info.setPName("main");
        return stage_info;
    }

    std::vector<char> Shader::read_file(const std::string &file_name)
    {
        std::ifstream file(file_name, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + file_name + "!");
        }

        size_t file_size = static_cast<size_t> (file.tellg());
        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);

        file.close();
        return buffer;
    }
}

#pragma clang diagnostic pop
