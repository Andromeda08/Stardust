#pragma once

#include <iostream>
#include <memory>
#include <ranges>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan.hpp>
#include <vk/Buffer.hpp>
#include <vk/Image.hpp>
#include <vk/Geometry.hpp>
#include <vk/Mesh.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Descriptors/DescriptorSetLayout.hpp>
#include <vk/Descriptors/DescriptorSets.hpp>
#include <vk/Descriptors/DescriptorWrites.hpp>
#include <vk/Pipelines/PipelineBuilder.hpp>
#include <vk/Pipelines/RenderPass.hpp>
#include <vk/Presentation/Swapchain.hpp>
#include <Scenes/Camera.hpp>
#include <Utility/Clock.hpp>
#include <Utility/Math.hpp>
#include <SSAO/SSAO.hpp>

namespace sd
{
    /**
     * @brief Scene base class with and example scene.
     */
    class Scene
    {
    protected:
        struct Descriptors
        {
            std::unique_ptr<DescriptorSets> sets;
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
            vk::DescriptorSetLayout layout;
        };
        struct Object
        {
            re::Mesh& mesh;
            Pipeline& material;
            glm::mat4 transform;
            glm::vec4 color;
            std::string name;
        };
        struct CameraUniformData
        {
            glm::mat4 view, proj, view_inverse, proj_inverse;
            glm::vec4 eye;
        };
        struct PushConstantData
        {
            glm::vec4 time;
            glm::mat4 model;
            glm::vec4 color;
        };
        enum   Bindings { eCamera };

        using CameraUB = re::UniformBuffer<CameraUniformData>;

    public:
        Scene(Swapchain& swapchain, const CommandBuffers& command_buffers);

        /**
         * Render the scene using rasterization.
         * @param frame Current frame (swapchain image index)
         * @param cmd Command buffer for current frame
         */
        virtual void rasterize(uint32_t frame, vk::CommandBuffer cmd);

        virtual void trace_rays(uint32_t frame, vk::CommandBuffer cmd);

        virtual void register_keybinds(GLFWwindow* window);

        void add_object(std::string const& mesh_id, std::string const& mat_id, std::string const& name);

        void add_object(std::string const& mesh_id, std::string const& mat_id, glm::mat4 const& model, glm::vec4 const& color, std::string const& name);

    protected:
        /**
         * Execute the specified render pass
         * @param render_pass Render pass
         * @param frame Current frame (swapchain image index)
         * @param cmd Command buffer for current frame
         * @param fun Lambda specifying the commands to execute
         */
        void render_pass(vk::RenderPass render_pass, uint32_t frame, vk::CommandBuffer cmd, const std::function<void()>& fun);

        /**
         * Updates the camera uniform buffer based on the currently active camera.
         * @param frame Current frame (swapchain image index)
         */
        void update_camera(uint32_t frame);

    private:
        /**
         * @brief Generate default scene assets
         * - Uniform buffer for Cameras
         * - Push Constants for Time, Object model matrix and color
         * - Default rendering pipeline with Blinn-Phong shading ["default"]
         * - Initial sphere mesh ["sphere"]
         */
        void gen_defaults();

    protected:
        uint32_t m_active_camera = 0;
        std::vector<std::unique_ptr<Camera>> m_cameras;

        std::unique_ptr<Clock> m_clock;
        std::vector<Object> m_objects;
        std::unordered_map<std::string, Pipeline> m_materials;
        std::unordered_map<std::string, std::shared_ptr<re::Mesh>> m_meshes;

        Descriptors m_descriptors;

        std::vector<std::unique_ptr<CameraUB>> m_uniform_camera;

        std::shared_ptr<RenderPass>      m_render_pass;
        std::unique_ptr<re::DepthBuffer> m_depth_buffer;
        std::array<vk::ClearValue, 2>    m_clear_values;

        Swapchain&            m_swapchain;
        CommandBuffers const& m_command_buffers;
        Device const&         m_device;
    };
}