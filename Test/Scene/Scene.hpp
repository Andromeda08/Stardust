#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Resources/CameraUniformData.hpp>
#include <Scene/Object.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>
#include <Vulkan/Image/DepthBuffer.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Rendering/Mesh.hpp>
#include <Vulkan/Rendering/Pipeline.hpp>
#include <Vulkan/Rendering/RenderPass.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sd
{
    class Scene
    {
    public:
        Scene(sdvk::CommandBuffers const& command_buffers, sdvk::Context const& context, sdvk::Swapchain const& swapchain);

        void rasterize(uint32_t current_frame, vk::CommandBuffer const& cmd);

    private:
        std::vector<Object> m_objects;
        std::unordered_map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;
        std::unordered_map<std::string, sdvk::Pipeline> m_pipelines;

        std::unique_ptr<sdvk::Tlas> m_tlas;
        std::unique_ptr<sdvk::Descriptor> m_descriptor;

        std::vector<std::unique_ptr<sdvk::Buffer>> m_uniform_camera;

        struct Rendering {
            std::unique_ptr<sdvk::DepthBuffer> depth_buffer;
            std::unique_ptr<sdvk::RenderPass>  render_pass;
            std::array<vk::ClearValue,  2>     clear_values;
            std::array<vk::Framebuffer, 2>     framebuffers;
        } m_rendering;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context&        m_context;
        const sdvk::Swapchain&      m_swapchain;
    };
}