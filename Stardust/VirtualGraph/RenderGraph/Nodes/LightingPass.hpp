#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <Resources/CameraUniformData.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>
#include <Vulkan/Buffer.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula::RenderGraph
{
    enum class LightingPassShadowMode
    {
        eRayQuery,
        eShadowMaps,
        eNone,
    };

    std::string to_string(LightingPassShadowMode shadow_mode);

    struct LightingPassOptions
    {
        bool include_ao {false};
        bool include_aa {false};
        bool with_shadows {true};
    };

    struct LightingPassUniform
    {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 view_inverse;
        glm::mat4 proj_inverse;
        glm::vec4 eye;
    };

    struct LightingPassPushConstant
    {
        LightingPassPushConstant(const LightingPassOptions& options)
        {
            flags_a = { options.with_shadows, 0, 0, 0 };
        }

        /*
         * [0]: Enable RayQuery shadows
         */
        glm::ivec4 flags_a;
        glm::vec4  light_pos;
    };

    class LightingPass : public Node
    {
    public:
        explicit LightingPass(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~LightingPass() override = default;

    private:
        void _update_descriptor(uint32_t current_frame);

        LightingPassOptions m_options;

        struct Renderer
        {
            std::shared_ptr<Descriptor> descriptor;
            std::shared_ptr<Framebuffer> framebuffers;
            vk::Pipeline pipeline;
            vk::PipelineLayout pipeline_layout;
            vk::RenderPass render_pass;

            std::array<vk::ClearValue, 1> clear_values;
            uint32_t frames_in_flight;
            vk::Extent2D render_resolution;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform;
            std::vector<vk::Sampler> samplers;
        } m_renderer;

        const sdvk::Context& m_context;

    public:
        const std::vector<ResourceSpecification>& get_resource_specs() const override
        {
            return s_resource_specs;
        }

        static const std::vector<ResourceSpecification> s_resource_specs;
    };
}