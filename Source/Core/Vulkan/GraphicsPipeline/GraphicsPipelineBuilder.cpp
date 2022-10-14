#include "GraphicsPipelineBuilder.hpp"

GraphicsPipelineBuilder::GraphicsPipelineBuilder(const Device& device,
                                                 const vk::PipelineLayout& pipeline_layout,
                                                 const RenderPass& render_pass,
                                                 GraphicsPipelineState& pipeline_state)
: m_device(device)
, m_pipeline_state(pipeline_state)
{
    m_create_info.setLayout(pipeline_layout);
    m_create_info.setRenderPass(render_pass.handle());
    m_create_info.setPNext(nullptr);

    m_create_info.setPInputAssemblyState(&m_pipeline_state.input_assembly_state);
    m_create_info.setPRasterizationState(&m_pipeline_state.rasterization_state);
    m_create_info.setPMultisampleState(&m_pipeline_state.multisample_state);
    m_create_info.setPDepthStencilState(&m_pipeline_state.depth_stencil_state);
    m_create_info.setPViewportState(&m_pipeline_state.viewport_state);
    m_create_info.setPDynamicState(&m_pipeline_state.dynamic_state);
    m_create_info.setPColorBlendState(&m_pipeline_state.color_blend_state);
    m_create_info.setPVertexInputState(&m_pipeline_state.vertex_input_state);
}

vk::Pipeline GraphicsPipelineBuilder::create_pipeline(vk::PipelineCache pipeline_cache)
{
    update();

    vk::Result result;
    vk::Pipeline pipeline;

    std::tie( result, pipeline ) = m_device.handle().createGraphicsPipeline(pipeline_cache, m_create_info);

    return pipeline;
}

void GraphicsPipelineBuilder::add_shader(const ShaderModule& shader_module)
{
    vk::PipelineShaderStageCreateInfo create_info;

    create_info.setStage(shader_module.stage());
    create_info.setModule(shader_module.handle());
    create_info.setPName("main");

    m_shader_stages.push_back(create_info);
}
