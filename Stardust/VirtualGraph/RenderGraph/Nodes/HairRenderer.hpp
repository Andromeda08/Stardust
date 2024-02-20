#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include <Nebula/Descriptor.hpp>
#include <Nebula/Framebuffer.hpp>
#include <Scene/Camera.hpp>
#include <Vulkan/Context.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>
#include <VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp>

namespace Nebula::RenderGraph
{
    namespace Editor
    {
        class HairRendererEditorNode final : public Node
        {
        public:
            HairRendererEditorNode();
        };
    }

    struct HairRendererPushConstant
    {
        glm::mat4   model;
        glm::ivec4  params;
        glm::vec4   offset;
        uint64_t    vertex_address;
    };

    class HairRenderer : public Node
    {
    public:
        explicit HairRenderer(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~HairRenderer() override = default;

    private:
        void update_descriptor(uint32_t current_frame);

        void init_single_strand_pipeline();

        using BufferPtr = std::shared_ptr<sdvk::Buffer>;

        std::array<vk::ClearValue, 2>   m_clear_values;
        std::shared_ptr<Descriptor>     m_descriptor;
        uint32_t                        m_frames_in_flight;
        std::shared_ptr<Framebuffer>    m_framebuffers;
        vk::Pipeline                    m_pipeline;
        vk::PipelineLayout              m_pipeline_layout;
        vk::RenderPass                  m_render_pass;
        vk::Extent2D                    m_render_resolution;
        std::vector<BufferPtr>          m_camera_uniform;

        const sdvk::Context&            m_context;

        static constexpr std::string id_scene_data  = "Scene Data";
        static constexpr std::string id_output      = "Output Image";
        static constexpr std::string id_depth       = "Depth Buffer";

        DEF_RESOURCE_REQUIREMENTS();
    };
}