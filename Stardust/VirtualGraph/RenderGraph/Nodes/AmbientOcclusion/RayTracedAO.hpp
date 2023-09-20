#pragma once

#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <Vulkan/Buffer.hpp>
#include "AmbientOcclusionOptions.hpp"
#include "AmbientOcclusionStrategy.hpp"

namespace Nebula::RenderGraph
{
    struct RayTracedAOOptions
    {
        RayTracedAOOptions() = default;

        explicit RayTracedAOOptions(const AmbientOcclusionOptions& options)
        {
            ao_radius = options.radius;
            ao_samples = options.samples;
            ao_power = options.power;
            max_samples = options.max_samples;
        }

        float      ao_radius      {3.0f};
        int32_t    ao_samples     {16};
        float      ao_power       {2.0f};
        int32_t    distance_based {0};
        int32_t    max_samples    {50000};
        int32_t    cur_samples    {0};
    };

    using RayTracedAOPushConsant = RayTracedAOOptions;

    class RayTracedAO : public AmbientOcclusionStrategy
    {
    public:
        explicit RayTracedAO(const sdvk::Context& context,
                             std::map<std::string, std::shared_ptr<Resource>>& resources);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize(const AmbientOcclusionOptions& options) override;

    private:
        void _update_descriptor(uint32_t current_frame);

        RayTracedAOOptions m_options;

        struct Kernel
        {
            static constexpr int32_t    s_group_size {16};
            std::shared_ptr<Descriptor> descriptor;
            vk::Pipeline                pipeline;
            vk::PipelineLayout          pipeline_layout;
            std::vector<vk::Sampler>    samplers;
            uint32_t                    frames_in_flight;
            vk::Extent2D                render_resolution;
            int32_t                     frame {0};
        } m_kernel;


        glm::mat4 m_ref_mat = glm::mat4(1.0f);
    };
}