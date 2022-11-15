#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Utility/Macro.hpp"

class ShaderModule
{
public:
    NON_COPIABLE(ShaderModule)

    ShaderModule(vk::ShaderStageFlagBits shader_stage,
                 const std::string& file_name,
                 const Device& device);

    ~ShaderModule() { destroy(); }

    vk::ShaderModule handle() const { return m_shader_module; }

    vk::ShaderStageFlagBits stage() const { return m_shader_stage; }

    void destroy();

private:
    static std::vector<char> read_file(const std::string& file_name);

    vk::ShaderModule        m_shader_module;
    vk::ShaderStageFlagBits m_shader_stage;

    const Device& m_device;
};
