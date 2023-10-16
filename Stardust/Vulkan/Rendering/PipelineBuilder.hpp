#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Rendering/ShaderModule.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include <Vulkan/Rendering/PipelineState.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sdvk
{
    struct PipelineBuilder
    {
        explicit PipelineBuilder(Context const& context) : _context(context) {}

        PipelineBuilder& add_descriptor_set_layout(const vk::DescriptorSetLayout& dsl);

        PipelineBuilder& add_push_constant(const vk::PushConstantRange& pcr);

        PipelineBuilder& create_pipeline_layout();

        PipelineBuilder& enable_wireframe_mode();

        PipelineBuilder& add_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& viads);

        PipelineBuilder& add_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& vibds);

        PipelineBuilder& set_attachment_count(uint32_t c)
        {
            pipeline_state.blend_attachments.clear();
            for (int32_t i = 0; i < c; i++)
            {
                auto att = PipelineState::make_color_blend_attachment_state();
                pipeline_state.blend_attachments.push_back(att);
            }
            return *this;
        }

        PipelineBuilder& set_cull_mode(vk::CullModeFlagBits cull_mode)
        {
            pipeline_state.rasterization_state.setCullMode(cull_mode);
            return *this;
        }

        PipelineBuilder& set_sample_count(vk::SampleCountFlagBits sample_count)
        {
            _sample_count = sample_count;
            return *this;
        }

        PipelineBuilder& add_viewport(const vk::Viewport& viewport);

        PipelineBuilder& add_scissor(const vk::Rect2D& scissor);

        PipelineBuilder& add_shader(const std::string& shader_src, vk::ShaderStageFlagBits shader_stage);

        PipelineBuilder& make_rt_shader_groups();

        PipelineBuilder& with_name(std::string const& name)
        {
            _name = name;
            return *this;
        }

        std::tuple<vk::Pipeline, vk::PipelineLayout> create_graphics_pipeline(const vk::RenderPass& render_pass);

        std::tuple<vk::Pipeline, vk::PipelineLayout> create_compute_pipeline();

        std::tuple<vk::Pipeline, vk::PipelineLayout> create_ray_tracing_pipeline(int ray_recursion_depth);

    private:
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        std::vector<vk::PushConstantRange>   push_constant_ranges;

        PipelineState pipeline_state;
        std::vector<std::unique_ptr<ShaderModule>> shaders;
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
        vk::SampleCountFlagBits _sample_count;

        Pipeline pipeline;

        std::string _name;

        const Context& _context;
    };
}
