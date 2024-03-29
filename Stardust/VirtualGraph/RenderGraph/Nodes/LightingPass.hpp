#pragma once

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
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
        bool ambient_occlusion {false};
        bool enable_shadows {true};
        // bool debug_render_lights {false};
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
        explicit LightingPassPushConstant(const LightingPassOptions& options)
        {
            flags_a = { options.enable_shadows, 0, 0, 0 };
        }

        /*
         * [0]: Enable RayQuery shadows
         */
        glm::ivec4 flags_a {};
        glm::vec4  light_pos {};
    };

    class LightingPass final : public Node
    {
    public:
        explicit LightingPass(const sdvk::Context& context, const LightingPassOptions& params);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~LightingPass() override = default;

    private:
        void _update_descriptor(uint32_t current_frame);

        std::string _select_fragment_shader() const;

        LightingPassOptions m_params;

        struct Renderer
        {
            std::shared_ptr<Descriptor>                descriptor;
            std::shared_ptr<Framebuffer>               framebuffers;
            vk::Pipeline                               pipeline;
            vk::PipelineLayout                         pipeline_layout;
            vk::RenderPass                             render_pass;
            std::array<vk::ClearValue, 1>              clear_values;
            std::vector<vk::Sampler>                   samplers;
            std::vector<std::unique_ptr<sdvk::Buffer>> uniform;
            uint32_t                                   frames_in_flight;
            vk::Extent2D                               render_resolution;
        } m_renderer;

        const sdvk::Context& m_context;

        DEF_RESOURCE_REQUIREMENTS();
    };
}