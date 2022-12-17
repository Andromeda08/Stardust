#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Scenes/Camera.hpp>
#include <Utility/Math.hpp>
#include <vk/Buffer.hpp>
#include <vk/Geometry.hpp>
#include <vk/Image.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Mesh.hpp>
#include <vk/Texture.hpp>
#include <vk/UniformData.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Descriptors/DescriptorSetLayout.hpp>
#include <vk/Descriptors/DescriptorWrites.hpp>
#include <vk/Pipelines/PipelineBuilder.hpp>
#include <vk/Presentation/Swapchain.hpp>

namespace re
{
    class Scene
    {
    public:
        enum SceneBindings { eCamera, eDefaultTexture };
        struct CameraUniform
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 view_inverse;
            glm::mat4 proj_inverse;

            glm::mat4 model;
        };

        Scene(Swapchain& swapchain, const CommandBuffers& command_buffer);

        virtual void rasterize(size_t current_frame, vk::CommandBuffer cmd);

        Camera& camera() { return *m_camera3d; }

    private:
        void create_objects();

        void build_default_rendering_pipeline(const std::vector<std::string>& default_shaders);

        void update_camera(uint32_t index) const;

    private:
        template <typename T>
        using u_ptr = std::unique_ptr<T>;

        const CommandBuffers& m_command_buffers;
        const Device&         m_device;
        Swapchain&            m_swapchain;

        std::vector<u_ptr<re::Mesh>> m_objects;
        std::vector<u_ptr<re::InstancedGeometry>> m_instanced_objects;

        u_ptr<Camera> m_camera3d;
        std::vector<u_ptr<re::UniformBuffer<CameraUniform>>> m_uniform_camera;

        std::vector<u_ptr<re::Texture>> m_textures;
        std::vector<u_ptr<re::Sampler>> m_samplers;

        std::vector<vk::DescriptorSetLayoutBinding> m_dslb;
        vk::DescriptorSetLayout                     m_dsl;
        u_ptr<DescriptorSets>                       m_descriptors;

        u_ptr<RenderPass>             m_render_pass;
        u_ptr<re::DepthBuffer>        m_depth_buffer;
        vk::Rect2D                    m_render_area;
        std::array<vk::ClearValue, 2> m_clear_values;

        Pipeline m_pipeline;

        std::default_random_engine m_rand;
    };
}
