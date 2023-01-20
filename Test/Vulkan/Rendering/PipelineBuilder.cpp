#include "PipelineBuilder.hpp"

namespace sdvk
{
    PipelineBuilder& PipelineBuilder::add_descriptor_set_layout(const vk::DescriptorSetLayout& dsl)
    {
        descriptor_set_layouts.push_back(dsl);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_push_constant(const vk::PushConstantRange& pcr)
    {
        push_constant_ranges.push_back(pcr);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::create_pipeline_layout()
    {
        vk::PipelineLayoutCreateInfo create_info;
        create_info.setSetLayoutCount(descriptor_set_layouts.size());
        create_info.setPSetLayouts(descriptor_set_layouts.data());
        create_info.setPushConstantRangeCount(push_constant_ranges.size());
        create_info.setPPushConstantRanges(push_constant_ranges.data());

        auto result = _context.device().createPipelineLayout(&create_info, nullptr, &pipeline.pipeline_layout);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::enable_wireframe_mode()
    {
        pipeline_state.rasterization_state.setPolygonMode(vk::PolygonMode::eLine);
        return *this;
    }

    PipelineBuilder&
    PipelineBuilder::add_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& viads)
    {
        pipeline_state.add_attribute_descriptions(viads);
        return *this;
    }

    PipelineBuilder&
    PipelineBuilder::add_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& vibds)
    {
        pipeline_state.add_binding_descriptions(vibds);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_viewport(const vk::Viewport& viewport)
    {
        pipeline_state.add_viewport(viewport);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_scissor(const vk::Rect2D& scissor)
    {
        pipeline_state.add_scissor(scissor);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_shader(const std::string& shader_src, vk::ShaderStageFlagBits shader_stage)
    {
        shaders.push_back(std::make_unique<ShaderModule>(shader_src, shader_stage, _context.device()));
        return *this;
    }

    Pipeline PipelineBuilder::create_graphics_pipeline(const RenderPass& render_pass)
    {
        if (!pipeline.pipeline_layout)
        {
            throw std::runtime_error("You must first create a vk::PipelineLayout object before creating a pipeline!");
        }

        pipeline_state.update();
        for (const auto& shader : shaders)
        {
            shader_stages.push_back(shader->stage_info());
        }

        vk::GraphicsPipelineCreateInfo create_info;

        create_info.setPInputAssemblyState(&pipeline_state.input_assembly_state);
        create_info.setPRasterizationState(&pipeline_state.rasterization_state);
        create_info.setPMultisampleState(&pipeline_state.multisample_state);
        create_info.setPDepthStencilState(&pipeline_state.depth_stencil_state);
        create_info.setPViewportState(&pipeline_state.viewport_state);
        create_info.setPDynamicState(&pipeline_state.dynamic_state);
        create_info.setPColorBlendState(&pipeline_state.color_blend_state);
        create_info.setPVertexInputState(&pipeline_state.vertex_input_state);

        create_info.setStageCount(static_cast<uint32_t>(shader_stages.size()));
        create_info.setPStages(shader_stages.data());

        create_info.setLayout(pipeline.pipeline_layout);
        create_info.setRenderPass(render_pass.handle());
        create_info.setPNext(nullptr);

        vk::Result result;
        std::tie( result, pipeline.pipeline ) = _context.device().createGraphicsPipeline(nullptr, create_info);

        return pipeline;
    }

    PipelineBuilder& PipelineBuilder::make_rt_shader_groups()
    {
        auto add_general = [this](size_t i){
            vk::RayTracingShaderGroupCreateInfoKHR g;
            g.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
            g.setGeneralShader(i);
            g.setClosestHitShader(VK_SHADER_UNUSED_KHR);
            g.setAnyHitShader(VK_SHADER_UNUSED_KHR);
            g.setIntersectionShader(VK_SHADER_UNUSED_KHR);
            shader_groups.push_back(g);
        };
        auto add_closest_hit = [this](size_t i){
            vk::RayTracingShaderGroupCreateInfoKHR g;
            g.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
            g.setGeneralShader(VK_SHADER_UNUSED_KHR);
            g.setClosestHitShader(i);
            g.setAnyHitShader(VK_SHADER_UNUSED_KHR);
            g.setIntersectionShader(VK_SHADER_UNUSED_KHR);
            shader_groups.push_back(g);
        };

        for (auto i = 0; i < shaders.size(); i++)
        {
            auto stage_info = shaders[i]->stage_info();
            shader_stages.push_back(stage_info);

            switch (stage_info.stage)
            {
                case vk::ShaderStageFlagBits::eRaygenKHR:
                case vk::ShaderStageFlagBits::eMissKHR:
                    add_general(i);
                    break;
                case vk::ShaderStageFlagBits::eClosestHitKHR:
                    add_closest_hit(i);
                    break;
            }
        }

        return *this;
    }

    Pipeline PipelineBuilder::create_ray_tracing_pipeline(int ray_recursion_depth)
    {
        if (!pipeline.pipeline_layout)
        {
            throw std::runtime_error("You must first create a vk::PipelineLayout object before creating a pipeline!");
        }

        vk::RayTracingPipelineCreateInfoKHR create_info;
        create_info.setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR | vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR);
        create_info.setStageCount(shader_stages.size());
        create_info.setPStages(shader_stages.data());
        create_info.setGroupCount(shader_groups.size());
        create_info.setPGroups(shader_groups.data());
        create_info.setMaxPipelineRayRecursionDepth(ray_recursion_depth);
        create_info.setLayout(pipeline.pipeline_layout);

        auto result = _context.device().createRayTracingPipelinesKHR(nullptr, nullptr, 1, &create_info, nullptr, &pipeline.pipeline);

        return pipeline;
    }
}