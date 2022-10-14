#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include "GraphicsPipelineState.hpp"
#include "RenderPass.hpp"
#include "ShaderModule.hpp"
#include "../Device.hpp"

class GraphicsPipelineBuilder
{
public:
    NON_COPIABLE(GraphicsPipelineBuilder)

    GraphicsPipelineBuilder(const Device& device,
                            const vk::PipelineLayout& pipeline_layout,
                            const RenderPass& render_pass,
                            GraphicsPipelineState& pipeline_state);

    vk::Pipeline create_pipeline(vk::PipelineCache pipeline_cache = nullptr);

    void add_shader(const ShaderModule& shader_module);

    void update()
    {
        m_create_info.setStageCount(static_cast<uint32_t>(m_shader_stages.size()));
        m_create_info.setPStages(m_shader_stages.data());
        m_pipeline_state.update();
    }

    GraphicsPipelineState& pipeline_state() { return m_pipeline_state; }

private:
    const Device& m_device;

    vk::GraphicsPipelineCreateInfo m_create_info;
    GraphicsPipelineState&         m_pipeline_state;

    std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stages;
    std::vector<ShaderModule>                      m_shader_modules;

    std::vector<vk::Format> m_formats;
};
