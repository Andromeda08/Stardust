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
    struct ScreenSpaceAOOptions {
        ScreenSpaceAOOptions() = default;

        explicit ScreenSpaceAOOptions(const AmbientOcclusionOptions& options)
        {
            sample_count = options.samples;
            noise_count = 32;
            radius = options.radius;
            bias = options.bias;
        }

        int32_t sample_count {64};
        int32_t noise_count {32};
        float radius {0.5f};
        float bias {0.025f};
    };

    struct ScreenSpaceAOUniform
    {
        explicit ScreenSpaceAOUniform(const ScreenSpaceAOOptions& options)
        {
            ints[0] = options.sample_count;
            floats[0] = options.radius;
            floats[1] = options.bias;
        }

        /* [0: sample_count] */
        glm::ivec4 ints {};

        /* [0: radius] [1: bias] */
        glm::vec4 floats {};

        std::array<glm::vec4, 64> samples {};
        std::array<glm::vec4, 32> noise {};
    };

    class ScreenSpaceAO : public AmbientOcclusionStrategy
    {
    public:
        explicit ScreenSpaceAO(const sdvk::Context& context,
                               std::map<std::string, std::shared_ptr<Resource>>& resources);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize(const AmbientOcclusionOptions& options) override;

    private:
        void _update_descriptor(uint32_t current_frame);

        ScreenSpaceAOOptions m_options;

        struct Kernel
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;
            std::array<vk::ClearValue, 1> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform_camera, uniform_ssao;
            std::vector<vk::Sampler> samplers;

            std::vector<glm::vec4> samples;
            std::vector<glm::vec4> noise;
        } m_kernel;

        std::uniform_real_distribution<float> m_random_floats;
        std::default_random_engine m_random;
    };
}