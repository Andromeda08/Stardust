#include "PipelineState.hpp"

PipelineState::PipelineState()
{
    blend_attachments = { make_color_blend_attachment_state() };
    dynamic_states = { vk::DynamicState::eScissor, vk::DynamicState::eViewport };

    input_assembly_state = make_input_assembly_state(vk::PrimitiveTopology::eTriangleList);
    rasterization_state  = make_rasterization_state(false);
    multisample_state    = make_multisample_state();
    depth_stencil_state  = make_depth_stencil_state();
    viewport_state       = make_viewport_state();
    dynamic_state        = make_dynamic_state();
    color_blend_state    = make_color_blend_state();
    vertex_input_state   = make_vertex_input_state();
}

void PipelineState::update()
{
    color_blend_state.setAttachmentCount(static_cast<uint32_t>(blend_attachments.size()));
    color_blend_state.setPAttachments(blend_attachments.data());

    dynamic_state.setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()));
    dynamic_state.setPDynamicStates(dynamic_states.data());

    vertex_input_state.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attribute_descriptions.size()));
    vertex_input_state.setPVertexAttributeDescriptions(attribute_descriptions.data());

    vertex_input_state.setVertexBindingDescriptionCount(static_cast<uint32_t>(binding_descriptions.size()));
    vertex_input_state.setPVertexBindingDescriptions(binding_descriptions.data());

    if (viewports.empty())
    {
        viewport_state.setViewportCount(1);
        viewport_state.setPViewports(nullptr);
    }
    else
    {
        viewport_state.setViewportCount(static_cast<uint32_t>(viewports.size()));
        viewport_state.setPViewports(viewports.data());
    }

    if (scissors.empty())
    {
        viewport_state.setScissorCount(1);
        viewport_state.setPScissors(nullptr);
    }
    else
    {
        viewport_state.setScissorCount(1);
        viewport_state.setPScissors(scissors.data());
    }
}

uint32_t PipelineState::add_binding_description(const vk::VertexInputBindingDescription &binding_description)
{
    binding_descriptions.push_back(binding_description);
    return static_cast<uint32_t>(binding_descriptions.size() - 1);
}

void PipelineState::add_binding_descriptions(const std::vector<vk::VertexInputBindingDescription> &descriptions)
{
    binding_descriptions.insert(std::end(binding_descriptions), std::begin(descriptions), std::end(descriptions));
}

uint32_t PipelineState::add_attribute_description(const vk::VertexInputAttributeDescription &attribute_description)
{
    attribute_descriptions.push_back(attribute_description);
    return static_cast<uint32_t>(attribute_descriptions.size() - 1);
}

void PipelineState::add_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription> &descriptions)
{
    attribute_descriptions.insert(std::end(attribute_descriptions), std::begin(descriptions), std::end(descriptions));
}

uint32_t PipelineState::add_viewport(vk::Viewport viewport)
{
    viewports.push_back(viewport);
    return static_cast<uint32_t>(viewports.size() - 1);
}

uint32_t PipelineState::add_scissor(vk::Rect2D scissor)
{
    scissors.push_back(scissor);
    return static_cast<uint32_t>(scissors.size() - 1);
}

vk::PipelineRasterizationStateCreateInfo PipelineState::make_rasterization_state(bool wireframe)
{
    vk::PipelineRasterizationStateCreateInfo res;

    res.setPolygonMode(wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill);
    res.setCullMode(vk::CullModeFlagBits::eBack);
    res.setFrontFace(vk::FrontFace::eCounterClockwise);
    res.setDepthClampEnable(false);
    res.setDepthBiasEnable(false);
    res.setDepthBiasClamp(0.0f);
    res.setDepthBiasSlopeFactor(0.0f);
    res.setLineWidth(1.0f);
    res.setRasterizerDiscardEnable(false);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineInputAssemblyStateCreateInfo PipelineState::make_input_assembly_state(vk::PrimitiveTopology topology)
{
    vk::PipelineInputAssemblyStateCreateInfo res;

    res.setTopology(topology);
    res.setPrimitiveRestartEnable(false);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineMultisampleStateCreateInfo PipelineState::make_multisample_state()
{
    vk::PipelineMultisampleStateCreateInfo res;

    res.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    res.setSampleShadingEnable(false);
    res.setPSampleMask(nullptr);
    res.setAlphaToCoverageEnable(false);
    res.setAlphaToOneEnable(false);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineDepthStencilStateCreateInfo PipelineState::make_depth_stencil_state()
{
    vk::PipelineDepthStencilStateCreateInfo res;

    res.setDepthTestEnable(true);
    res.setDepthWriteEnable(true);
    res.setDepthCompareOp(vk::CompareOp::eLess);
    res.setDepthBoundsTestEnable(false);
    res.setStencilTestEnable(false);

    return res;
}

vk::PipelineViewportStateCreateInfo PipelineState::make_viewport_state()
{
    vk::PipelineViewportStateCreateInfo res;

    res.setViewportCount(0);
    res.setPViewports(nullptr);
    res.setScissorCount(0);
    res.setPScissors(nullptr);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineDynamicStateCreateInfo PipelineState::make_dynamic_state()
{
    vk::PipelineDynamicStateCreateInfo res;

    res.setDynamicStateCount(0);
    res.setPDynamicStates(nullptr);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineColorBlendStateCreateInfo PipelineState::make_color_blend_state()
{

    vk::PipelineColorBlendStateCreateInfo res;

    res.setLogicOp(vk::LogicOp::eClear);
    res.setLogicOpEnable(false);
    res.setAttachmentCount(0);
    res.setPAttachments(nullptr);
    res.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });
    res.setPNext(nullptr);

    return res;
}

vk::PipelineVertexInputStateCreateInfo PipelineState::make_vertex_input_state()
{
    vk::PipelineVertexInputStateCreateInfo res;

    res.setVertexAttributeDescriptionCount(0);
    res.setPVertexAttributeDescriptions(nullptr);
    res.setVertexBindingDescriptionCount(0);
    res.setPVertexBindingDescriptions(nullptr);
    res.setPNext(nullptr);

    return res;
}

vk::PipelineColorBlendAttachmentState
PipelineState::make_color_blend_attachment_state(vk::ColorComponentFlags color_write_mask, vk::Bool32 blend_enable,
                                                 vk::BlendFactor src_color_blend_factor,
                                                 vk::BlendFactor dst_color_blend_factor, vk::BlendOp color_blend_op,
                                                 vk::BlendFactor src_alpha_blend_factor,
                                                 vk::BlendFactor dst_alpha_blend_factor, vk::BlendOp alpha_blend_op)
{
    vk::PipelineColorBlendAttachmentState res;

    res.setColorWriteMask(color_write_mask);
    res.setBlendEnable(blend_enable);
    res.setSrcColorBlendFactor(src_color_blend_factor);
    res.setDstColorBlendFactor(dst_color_blend_factor);
    res.setColorBlendOp(color_blend_op);
    res.setSrcAlphaBlendFactor(src_alpha_blend_factor);
    res.setDstAlphaBlendFactor(dst_alpha_blend_factor);
    res.setAlphaBlendOp(alpha_blend_op);

    return res;
}
