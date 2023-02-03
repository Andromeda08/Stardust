#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <Rendering/AmbientOcclusion.hpp>
#include <Rendering/RenderSettings.hpp>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/Buffer.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Image/DepthBuffer.hpp>
#include <Vulkan/Image/Sampler.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Rendering/Mesh.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sd
{
    class Scene
    {
    public:
        Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context, const sdvk::Swapchain& swapchain);

        virtual void rasterize(uint32_t current_frame, vk::CommandBuffer const& command_buffer);

        virtual void register_keybinds(GLFWwindow* p_window);

    private:
        void update_offscreen_descriptors(uint32_t current_frame);

        void update_composite_descriptors();

        void load_objects_from_json(std::string const& objects_json);

        void load_sponza();

    private:
        std::vector<Object> m_objects;
        std::unordered_map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;
        std::unordered_map<std::string, sdvk::Pipeline> m_pipelines;

        std::shared_ptr<sdvk::Tlas> m_tlas;
        std::unique_ptr<Camera> m_camera;
        std::vector<std::unique_ptr<sdvk::Buffer>> m_uniform_camera;

        std::array<vk::Framebuffer, 2> m_framebuffers;

        RenderSettings m_render_settings = {};

        struct OffscreenRenderTarget
        {
            std::unique_ptr<sdvk::Descriptor>  descriptor;
            std::unique_ptr<sdvk::Image>       output;
            std::shared_ptr<sdvk::Image>       g_buffer;
            std::unique_ptr<sdvk::DepthBuffer> depth_image;
            std::array<vk::ClearValue, 3>      clear_values;
            vk::Framebuffer                    framebuffer;
            vk::RenderPass                     render_pass;
        } m_offscreen_render_target;

        struct CompositeRenderTarget
        {
            std::unique_ptr<sdvk::Descriptor> descriptor;
            std::array<vk::ClearValue, 2>     clear_values;
            vk::RenderPass                    render_pass;
            vk::Sampler                       sampler;
        } m_composite_render_target;

        std::unique_ptr<AmbientOcclusion> m_ambient_occlusion;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context&        m_context;
        const sdvk::Swapchain&      m_swapchain;
    };
}