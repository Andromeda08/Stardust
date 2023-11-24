#pragma once

#include <memory>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/Context.hpp>
#include <VirtualGraph/Editor/Node.hpp>
#include <VirtualGraph/RenderGraph/Nodes/Node.hpp>

namespace Nebula::RenderGraph
{
    namespace Editor
    {
        class MeshGBufferPassEditorNode final : public Node
        {
        public:
            MeshGBufferPassEditorNode();
        };
    }

    struct Meshlet
    {
        uint32_t vertex[64]{};
        uint32_t index[126]{};
        uint32_t vertex_count {0};
        uint32_t index_count {0};
    };

    struct MShGBufferPushConstant
    {
        glm::mat4 model;
        glm::vec4 color;
        uint64_t  vertex_address;
        uint64_t  meshlets_address;
    };

    struct CameraDataUniform
    {
        sd::CameraUniformData current;
        sd::CameraUniformData previous;
    };


    class MeshGBufferPass final : public Node
    {
    public:
        explicit MeshGBufferPass(const sdvk::Context& context);

        void execute(const vk::CommandBuffer& command_buffer) override;

        void initialize() override;

        ~MeshGBufferPass() override = default;

        const std::vector<ResourceSpecification>& get_resource_specs() const override
        {
            return s_resource_specs;
        }

    private:
        void update_descriptor(uint32_t current_frame);

        using BufferPtr = std::shared_ptr<sdvk::Buffer>;

        struct Renderer
        {
            std::shared_ptr<Descriptor>   descriptor;
            std::shared_ptr<Framebuffer>  framebuffers;
            vk::Pipeline                  pipeline;
            vk::PipelineLayout            pipeline_layout;
            vk::RenderPass                render_pass;
            std::array<vk::ClearValue, 5> clear_values;
            uint32_t                      frames_in_flight;
            vk::Extent2D                  render_resolution;
            std::vector<BufferPtr>        camera_uniform;
            sd::CameraUniformData         previous_frame_camera_state;
        } m_renderer;

        const sdvk::Context& m_context;

        static constexpr std::string s_shader_name      = "rg_draw_mesh";
        static constexpr std::string id_scene_data      = "Scene Data";
        static constexpr std::string id_position_buffer = "Position Buffer";
        static constexpr std::string id_normal_buffer   = "Normal Buffer";
        static constexpr std::string id_albedo_buffer   = "Albedo Buffer";
        static constexpr std::string id_depth_buffer    = "Depth Buffer";
        static constexpr std::string id_motion_vectors  = "Motion Vectors";


        friend Editor::MeshGBufferPassEditorNode;
        static const std::vector<ResourceSpecification> s_resource_specs;
    };
}
