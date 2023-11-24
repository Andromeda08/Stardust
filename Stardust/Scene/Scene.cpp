#include "Scene.hpp"

#include <format>
#include <Application/Application.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Scene/Transform.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Raytracing/Tlas.hpp>
#include <Vulkan/Rendering/Mesh.hpp>
#include <Window/Window.hpp>

namespace sd
{
    Scene::Scene(const sdvk::CommandBuffers& command_buffers,
                 const sdvk::Context&        context)
    : m_command_buffers(command_buffers), m_context(context)
    {
        add_defaults();
        default_init();
        create_object_description_buffer(command_buffers);
        create_acceleration_structure();
    }

    Scene::Scene(const std::function<void()>& init,
                 const sdvk::CommandBuffers&  command_buffers,
                 const sdvk::Context&         context)
    : m_command_buffers(command_buffers), m_context(context)
    {
        add_defaults();
        init();
        create_object_description_buffer(command_buffers);
        create_acceleration_structure();
    }

    void Scene::add_defaults()
    {
        auto res = Application::s_extent;
        m_camera = std::make_shared<Camera>(
            glm::ivec2 { res.width, res.height },
            glm::vec3 { 5.f, 5.f, 5.f }
        );

        m_meshes["cube"] = std::make_shared<sdvk::Mesh>(
            new primitives::Cube(),
            m_command_buffers,
            m_context,
            "cube"
        );

        m_meshes["sphere"] = std::make_shared<sdvk::Mesh>(
            new primitives::Sphere(1.0f, 250),
            m_command_buffers,
            m_context,
            "sphere"
        );
    }

    void Scene::create_acceleration_structure()
    {
        if (m_context.is_raytracing_capable())
        {
            m_acceleration_structure = sdvk::Tlas::Builder()
                .create(m_objects, m_command_buffers, m_context);
        }
    }

    void Scene::default_init()
    {
        static std::vector<std::pair<glm::vec4, glm::vec4>> color_pool = {
            {{1, 1, 0, 1}, {0, 0.8f, 1, 1}},
            {{1, 0, 0.25f, 1}, {1, 0, 0.85f, 1}},
            {{0.2f, 1, 0.2f, 1}, {0.1f, 0.8f, 0.9f, 1}},
            {{1, 0.1f, 0.1f, 1}, {1, 0.5f, 0, 1}}
        };

        auto randf = [](float lo = 0.0f, float hi = 1.0f){ return lo + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(hi - lo))); };

         Object plane = {};
         plane.color = { .5f, .5f, .5f, 1.f };
         plane.mesh = m_meshes["cube"];
         plane.name = "Object 1";
         plane.transform.scale = { 192.f, 0.05f, 192.f };

         m_objects.push_back(plane);

         Object bg1 = {};
         bg1.color = { .05f, .05f, .1f, 1.f };
         bg1.mesh = m_meshes["cube"];
         bg1.name = "Object 2";
         bg1.transform.scale = { 0.05f, 1500.0f, 1500.0f };
         bg1.transform.position = { 96, 0, 0 };

         m_objects.push_back(bg1);

         Object bg2 = {};
         bg2.color = { .05f, .05f, .1f, 1.f };
         bg2.mesh = m_meshes["cube"];
         bg2.name = "Object 3";
         bg2.transform.scale = { 1500.0f, 1500.0f, 0.05f };
         bg2.transform.position = { 0, 0, 96 };

         m_objects.push_back(bg2);

         ObjDescription plane_desc;
         plane_desc.vertex_buffer = plane.mesh->vertex_buffer().address();
         plane_desc.index_buffer  = plane.mesh->index_buffer().address();
         m_obj_descriptions.push_back(plane_desc);

         ObjDescription bg_dsc1;
         bg_dsc1.vertex_buffer = bg1.mesh->vertex_buffer().address();
         bg_dsc1.index_buffer  = bg1.mesh->index_buffer().address();
         m_obj_descriptions.push_back(bg_dsc1);

         ObjDescription bg_dsc2;
         bg_dsc2.vertex_buffer = bg2.mesh->vertex_buffer().address();
         bg_dsc2.index_buffer  = bg2.mesh->index_buffer().address();
         m_obj_descriptions.push_back(bg_dsc2);

         for (uint32_t i = 0; i < 1024; i++)
         {
             Transform transform = {};
             transform.scale = glm::vec3(
                     static_cast<float>(rand() % 4 + 1),
                     static_cast<float>(rand() % 16 + 1),
                     static_cast<float>(rand() % 5 + 1)
                 );
             transform.position = glm::vec3(
                     static_cast<float>(rand() % 192 - 96) + randf(),
                     randf(-0.05f, 0.0f),
                     static_cast<float>(rand() % 192 - 96) + randf()
                 );

             Object obj = {};
             auto idx = std::rand() % color_pool.size();
             obj.color = (rand() % 2 == 0) ? color_pool[idx].first : color_pool[idx].second;
             obj.mesh = m_meshes["cube"];
             obj.name = std::format("Object {}", std::to_string(m_objects.size() + 1));
             obj.transform = transform;
             m_objects.push_back(obj);

             ObjDescription obj_desc;
             obj_desc.vertex_buffer = obj.mesh->vertex_buffer().address();
             obj_desc.index_buffer  = obj.mesh->index_buffer().address();
             m_obj_descriptions.push_back(obj_desc);
         }
    }

    void Scene::key_handler(const Window& window)
    {
        m_camera->register_keys(window.handle());
    }

    void Scene::mouse_handler(const Window& window)
    {
        m_camera->register_mouse(window.handle());
    }

    void Scene::create_object_description_buffer(const sdvk::CommandBuffers& command_buffers)
    {
        m_obj_desc_buffer = sdvk::Buffer::Builder()
            .with_name("Scene: Object Description Buffer")
            .with_size(sizeof(ObjDescription) * m_obj_descriptions.size())
            .as_storage_buffer()
            .create_with_data(m_obj_descriptions.data(), command_buffers, m_context);
    }
}