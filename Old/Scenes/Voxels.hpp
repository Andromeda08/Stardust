#pragma once

#include <glm/gtc/noise.hpp>
#include <Scenes/Scene.hpp>
#include <vk/InstancedGeometry.hpp>

namespace sd
{
    /**
     * @brief Instanced Rendering demo
     */
    class Voxels : public Scene
    {
    public:
        Voxels(glm::ivec2 dim, Swapchain& swapchain, CommandBuffers const& command_buffers);

        void rasterize(uint32_t frame, vk::CommandBuffer cmd) override;

        void register_keybinds(GLFWwindow* window) override;

    private:
        void generate_height_map();

        void generate_terrain();

    private:
        const glm::ivec2 m_dimensions;

        using HeightMap = std::vector<std::vector<int>>;
        HeightMap m_height_map;

        // Voxels are rendered using instanced rendering
        std::vector<re::InstanceData>          m_instance_data;
        std::unique_ptr<re::InstancedGeometry> m_voxels;
    };
}