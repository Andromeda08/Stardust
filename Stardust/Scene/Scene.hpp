#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Image/DepthBuffer.hpp>
#include <Vulkan/Image/Image.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Rendering/Mesh.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sd
{
    class Scene
    {
    public:
        Scene(sdvk::CommandBuffers const& command_buffers, sdvk::Context const& context, sdvk::Swapchain const& swapchain);

        void rasterize(uint32_t current_frame, vk::CommandBuffer const& cmd);

        void load_objects_from_json(std::string const& objects_json);

        void init_rtao();
        void run_compute(const vk::CommandBuffer& cmd);

        void register_keybinds(GLFWwindow* p_window);

    private:
        std::vector<Object> m_objects;
        std::unordered_map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;
        std::unordered_map<std::string, sdvk::Pipeline> m_pipelines;

        std::unique_ptr<sdvk::Tlas> m_tlas;
        std::unique_ptr<sdvk::Descriptor> m_descriptor;

        std::unique_ptr<Camera> m_camera;
        std::vector<std::unique_ptr<sdvk::Buffer>> m_uniform_camera;

        struct RTAO {
            vk::Sampler sampler;
            std::unique_ptr<sdvk::Image> gbuffer, aobuffer, offscreen;
            std::unique_ptr<sdvk::DepthBuffer> offscreen_depth;
            vk::RenderPass render_pass;
            vk::Framebuffer framebuffer;
            std::array<vk::ClearValue, 3> clear_values;
            std::unique_ptr<sdvk::Descriptor> comp_desc;
            sdvk::Pipeline compute, pipeline;
            struct AoParams {
                float   rtao_radius {8.0f};
                int32_t rtao_samples {64};
                float   rtao_power {2.0f};
                int32_t rtao_distance_based {1};
                int32_t max_samples {100000};
                int32_t frame {0};
            } params;
            uint32_t frame {0};
        } m_rtao;
        struct Post {
            std::unique_ptr<sdvk::Descriptor> post_desc;
            sdvk::Pipeline post;
            vk::RenderPass render_pass;
            vk::Framebuffer framebuffer;
            std::unique_ptr<sdvk::Image> composite;
        } m_post;

        struct Rendering {
            std::unique_ptr<sdvk::Image>       multisampling_buffer;
            std::unique_ptr<sdvk::DepthBuffer> depth_buffer;
            vk::RenderPass msaa;
            std::array<vk::ClearValue,  2>     clear_values;
            std::array<vk::Framebuffer, 2>     framebuffers;
        } m_rendering;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context&        m_context;
        const sdvk::Swapchain&      m_swapchain;
    };
}