#pragma once

#include <memory>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Scenes/Camera.hpp>
#include <vk/Buffer.hpp>
#include <vk/InstancedGeometry.hpp>
#include <vk/Pipelines/PipelineBuilder.hpp>
#include <vk/Pipelines/RenderPass.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Descriptors/DescriptorSets.hpp>
#include <vk/Device/Device.hpp>
#include <vk/Presentation/Swapchain.hpp>

class TerrainScene
{
public:
    enum SceneBindings { eCamera };
    struct UniformCamera {
        glm::mat4 view, projection, view_inverse, proj_inverse;
        glm::vec4 camera_pos;
    };

    TerrainScene(glm::ivec2 dim, Swapchain& swapchain, const CommandBuffers& command_buffer);

    void rasterize(uint32_t current_frame, vk::CommandBuffer cmd);

    void scene_key_bindings(GLFWwindow* window);

private:
    void generate_height_map();

    void generate_terrain();

    void build_pipeline();

    void update_camera(uint32_t index) const;

private:
    using CameraUB  = re::UniformBuffer<UniformCamera>;
    using HeightMap = std::vector<std::vector<int>>;

    HeightMap  m_height_map;
    glm::ivec2 m_dimensions;

    std::unique_ptr<Camera>                m_camera;
    std::vector<re::InstanceData>          m_instance_data;
    std::unique_ptr<re::InstancedGeometry> m_objects;
    std::vector<std::unique_ptr<CameraUB>> m_uniform;

    std::vector<vk::DescriptorSetLayoutBinding> m_dslb;
    vk::DescriptorSetLayout                     m_dsl;
    std::unique_ptr<DescriptorSets>             m_descriptors;

    std::unique_ptr<RenderPass>      m_render_pass;
    std::unique_ptr<re::DepthBuffer> m_depth_buffer;
    std::array<vk::ClearValue, 2>    m_clear_values;
    vk::Rect2D                       m_render_area;

    Pipeline m_pipeline;

    const CommandBuffers& m_command_buffers;
    const Device&         m_device;
          Swapchain&      m_swapchain;
};
