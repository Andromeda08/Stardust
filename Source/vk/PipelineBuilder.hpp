#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Device.hpp>
#include <Vulkan/Swapchain.hpp>
#include <Vulkan/GraphicsPipeline/GraphicsPipelineState.hpp>
#include <Vulkan/GraphicsPipeline/RenderPass.hpp>
#include <vk/Pipeline.hpp>
#include <vk/Shader.hpp>


struct PipelineBuilder
{
    explicit PipelineBuilder(const Device& device) : _device(device) {}

    PipelineBuilder& add_descriptor_set_layout(const vk::DescriptorSetLayout& dsl);
    PipelineBuilder& add_push_constant(const vk::PushConstantRange& pcr);
    PipelineBuilder& create_pipeline_layout();

    PipelineBuilder& enable_wireframe_mode();
    PipelineBuilder& add_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& viads);
    PipelineBuilder& add_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& vibds);
    PipelineBuilder& add_viewport(const vk::Viewport& viewport);
    PipelineBuilder& add_scissor(const vk::Rect2D& scissor);

    PipelineBuilder& add_shader(const std::string& shader_src, vk::ShaderStageFlagBits shader_stage);

    /**
     * @brief Creates a rasterization pipeline with the specified render pass.
     */
    Pipeline create_graphics_pipeline(const RenderPass& render_pass);

    PipelineBuilder& make_rt_shader_groups();

    /**
     * @brief Creates a ray tracing pipeline with the specified maximum ray recursion depth.
     */
    Pipeline create_ray_tracing_pipeline(int ray_recursion_depth);

private:
    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
    std::vector<vk::PushConstantRange>   push_constant_ranges;

    GraphicsPipelineState pipeline_state;
    std::vector<std::unique_ptr<re::Shader>> shaders;
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;

    Pipeline pipeline;

    const Device& _device;
};