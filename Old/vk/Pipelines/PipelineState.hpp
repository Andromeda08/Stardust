#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct PipelineState
{
    /**
     * @brief Generate a basic default graphics pipeline
     */
    PipelineState();

    /**
     * @brief Update dynamic states, viewports, scissors,
     * vertex binding and attachment descriptions and color blend attachments
     */
    void update();

    /**
     * @brief Add a vertex input binding description
     * @return index of added description
     */
    uint32_t add_binding_description(const vk::VertexInputBindingDescription& binding_description);

    /**
     * @brief Add a list of vertex input binding descriptions
     */
    void add_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& descriptions);

    /**
     * @brief Add a vertex input attribute description
     * @return index of added description
     */
    uint32_t add_attribute_description(const vk::VertexInputAttributeDescription& attribute_description);

    /**
     * @brief Add a list of vertex input attribute descriptions
     */
    void add_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& descriptions);

    /**
     * @brief Add a viewport
     * @return index of added viewport
     */
    uint32_t add_viewport(vk::Viewport viewport);

    /**
     * @brief Add a scissor
     * @return index of added scissor
     */
    uint32_t add_scissor(vk::Rect2D scissor);


private:
    #pragma region functions_create_state_default

    static inline vk::PipelineRasterizationStateCreateInfo make_rasterization_state(bool wireframe = false);

    static inline vk::PipelineInputAssemblyStateCreateInfo make_input_assembly_state(vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList);

    static inline vk::PipelineMultisampleStateCreateInfo make_multisample_state();

    static inline vk::PipelineDepthStencilStateCreateInfo make_depth_stencil_state();

    static inline vk::PipelineViewportStateCreateInfo make_viewport_state();

    static inline vk::PipelineDynamicStateCreateInfo make_dynamic_state();

    static inline vk::PipelineColorBlendStateCreateInfo make_color_blend_state();

    static inline vk::PipelineVertexInputStateCreateInfo make_vertex_input_state();

    static inline vk::PipelineColorBlendAttachmentState make_color_blend_attachment_state(
        vk::ColorComponentFlags color_write_mask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        vk::Bool32 blend_enable = false,
        vk::BlendFactor src_color_blend_factor = vk::BlendFactor::eOne,
        vk::BlendFactor dst_color_blend_factor = vk::BlendFactor::eZero,
        vk::BlendOp color_blend_op = vk::BlendOp::eAdd,
        vk::BlendFactor src_alpha_blend_factor = vk::BlendFactor::eOne,
        vk::BlendFactor dst_alpha_blend_factor = vk::BlendFactor::eZero,
        vk::BlendOp alpha_blend_op = vk::BlendOp::eAdd
    );

    #pragma endregion

public:
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
    vk::PipelineRasterizationStateCreateInfo rasterization_state;
    vk::PipelineMultisampleStateCreateInfo   multisample_state;
    vk::PipelineDepthStencilStateCreateInfo  depth_stencil_state;
    vk::PipelineViewportStateCreateInfo      viewport_state;
    vk::PipelineDynamicStateCreateInfo       dynamic_state;
    vk::PipelineColorBlendStateCreateInfo    color_blend_state;
    vk::PipelineVertexInputStateCreateInfo   vertex_input_state;

private:
    std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments;

    std::vector<vk::VertexInputBindingDescription>   binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;

    std::vector<vk::DynamicState> dynamic_states;
    std::vector<vk::Viewport>     viewports;
    std::vector<vk::Rect2D>       scissors;
};
