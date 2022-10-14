#include "ShaderModule.hpp"

#include <fstream>

ShaderModule::ShaderModule(vk::ShaderStageFlagBits shader_stage,
                           const std::string& file_name,
                           const Device& device)
: m_device(device)
, m_shader_stage(shader_stage)
{
    auto source_code = ShaderModule::read_file(file_name);
    vk::ShaderModuleCreateInfo create_info;
    create_info.setCodeSize(sizeof(char) * source_code.size());
    create_info.setPCode(reinterpret_cast<const uint32_t*>(source_code.data()));
    auto result = m_device.handle().createShaderModule(&create_info, nullptr, &m_shader_module);
}

void ShaderModule::destroy()
{
    if (m_shader_module)
    {
        m_device.handle().destroyShaderModule(m_shader_module, nullptr);
        m_shader_module = nullptr;
    }
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
