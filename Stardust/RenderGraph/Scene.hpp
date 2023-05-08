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

            auto gradient = [&](uint32_t x, uint32_t y, uint32_t z, uint32_t steps){
                glm::vec4 color = { 0, 0, 0, 1 };
                auto s = static_cast<float>(steps);
                color.x = static_cast<float>(x) * (1.0f / s);
                color.y = static_cast<float>(y) * (1.0f / s);
                color.z = static_cast<float>(z) * (1.0f / s);

                color /= 2.0f;
                color += 0.25f;
                color.w = 1.0f;

                return color;
            };

            int32_t dim = 5;
            for (int32_t i = 0; i < dim; i++) {
                for (int32_t j = 0; j < dim; j++) {
                    for (int32_t k = 0; k < dim; k++) {
                        Object obj;
                        obj.name = "cube" + std::to_string(i);
                        obj.pipeline = "default";
                        obj.mesh = m_meshes["cube"];

                        glm::vec4 color { 0, 1, 1, 1 };
                        color.x = (float) (i) / ((float) dim);
                        color.y = 0.25f;
                        color.z = 1.0f - (float) (k) / ((float) dim);


                        obj.color = gradient(i, j, k, dim);
                        obj.transform.position = {
                                (float) i * 1.25f,
                                (float) j * 1.25f + 0.5f,
                                (float) k * 1.25f
                        };
                        //m_objects.push_back(obj);
                    }
                }
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

        virtual void register_mousebinds(GLFWwindow* p_window)
        {
            m_camera->register_mouse(p_window);
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