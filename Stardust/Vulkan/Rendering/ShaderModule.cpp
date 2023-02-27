#include "ShaderModule.hpp"

#include <fstream>

namespace sdvk
{

    ShaderModule::ShaderModule(const std::string& source, vk::ShaderStageFlagBits shader_stage, const vk::Device& device)
    : stage(shader_stage)
    {
        auto shader_source_code = ShaderModule::read_file(source);

        vk::ShaderModuleCreateInfo create_info;
        create_info.setCodeSize(sizeof(char) * shader_source_code.size());
        create_info.setPCode(reinterpret_cast<const uint32_t*>(shader_source_code.data()));
        auto result = device.createShaderModule(&create_info, nullptr, &module);
    }

    vk::PipelineShaderStageCreateInfo ShaderModule::stage_info() const
    {
        vk::PipelineShaderStageCreateInfo stage_info;
        stage_info.setStage(stage);
        stage_info.setModule(module);
        stage_info.setPName("main");
        return stage_info;
    }

    std::vector<char> ShaderModule::read_file(const std::string& file_name)
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