#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <Scene/Camera.hpp>
#include <Scene/Object.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Presentation/Swapchain.hpp>

namespace sd::rg
{
    class Scene
    {
    public:
        Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context, const sdvk::Swapchain& swapchain)
        : m_command_buffers(command_buffers), m_context(context), m_swapchain(swapchain)
        {
            #pragma region cubes_scene
            auto extent = m_swapchain.extent();
            m_camera = std::make_unique<Camera>(glm::ivec2(extent.width, extent.height), glm::vec3(5.0f));

            std::string cube = "cube", sphere = "sphere";
            m_meshes[cube] = std::make_shared<sdvk::Mesh>(new primitives::Cube(), m_command_buffers, m_context, cube);
            m_meshes[sphere] = std::make_shared<sdvk::Mesh>(new primitives::Sphere(1.0f, 240), m_command_buffers, m_context, sphere);

            auto seed = static_cast <unsigned> (time(nullptr));
            srand (seed);
            std::cout << seed << std::endl;

            auto randf = [](float lo, float hi){ return lo + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(hi - lo))); };

            for (int32_t i = 0; i < 280; i++)
            {
                Object obj;
                obj.name = "cube" + std::to_string(i);
                obj.pipeline = "default";
                //obj.color = glm::vec4(randf(0.0f, 1.0f), randf(0.0f, 1.0f), randf(0.0f, 1.0f), 1.0f);
                obj.color = (rand() % 2 == 0) ? glm::vec4(1, 1, 0, 1) : glm::vec4(0, 0.8f, 1, 1);
                obj.transform.scale = glm::vec3((float) (rand() % 3 + 1), (float) (rand() % 7 + 1), (float) (rand() % 4 + 1));
                obj.transform.position = { (float) (rand() % 64 - 32) + randf(0.0f, 1.0f), randf(-0.05f, 0.0f), (float) (rand() % 64 - 32) + randf(0.0f, 1.0f)};
                obj.mesh = m_meshes["cube"];
                m_objects.push_back(obj);
            }

            Object obj;
            obj.name = "plane";
            obj.pipeline = "default";
            obj.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            obj.transform.scale = { 64, 0.05f, 64 };
            obj.mesh = m_meshes["cube"];
            m_objects.push_back(obj);

            m_tlas = sdvk::Tlas::Builder().create(m_objects, m_command_buffers, m_context);
            #pragma endregion


        }

        const std::vector<sd::Object>& objects() { return m_objects; }

        const std::shared_ptr<sd::Camera>& camera() { return m_camera; }

        const std::shared_ptr<sdvk::Tlas>& tlas() { return m_tlas; }

        virtual void register_keybinds(GLFWwindow* p_window)
        {
            m_camera->register_keys(p_window);
        }

    private:

    private:
        std::shared_ptr<sd::Camera> m_camera;
        std::vector<sd::Object>     m_objects;
        std::shared_ptr<sdvk::Tlas> m_tlas;

        std::unordered_map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context&        m_context;
        const sdvk::Swapchain&      m_swapchain;

        // RenderPath m_active_render_path;
    };
}