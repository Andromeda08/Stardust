#include "Scene.hpp"

#include <format>
#include <unordered_map>
#include <tiny_obj_loader.h>
#include <Application/Application.hpp>
#include <Resources/VertexData.hpp>
#include <Resources/Primitives/Cube.hpp>
#include <Resources/Primitives/Sphere.hpp>
#include <Hair/TestLineGeometry.hpp>
#include <Hair/Strand.hpp>
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
            "cube",
            3,
            3
        );

        m_meshes["sphere"] = std::make_shared<sdvk::Mesh>(
            new primitives::Sphere(1.0f, 250),
            m_command_buffers,
            m_context,
            "sphere",
            64,
            126
        );

        m_meshes["strand"] = std::make_shared<sdvk::Mesh>(
            new Nebula::Strand(6),
            m_command_buffers,
            m_context,
            "strand",
            64,
            126
        );

        m_meshes["curved_strand"] = std::make_shared<sdvk::Mesh>(
            Nebula::HairStrand::make_test_strand(),
            m_command_buffers,
            m_context,
            "curved_strand",
            64,
            126
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
         for (int32_t i = 0; i < 1; i++) {
             Object strand {};
             strand.color = { 0.75f, 0.75f, 0.75f, 1.0f };
             strand.mesh = m_meshes["cube"];
             strand.transform.position = { 0, i * 2, 0 };
             strand.name = std::format("Object {}", i);

             m_objects.push_back(strand);

             ObjDescription strand_desc {};
             strand_desc.vertex_buffer = strand.mesh->vertex_buffer().address();
             strand_desc.index_buffer  = strand.mesh->index_buffer().address();
             m_obj_descriptions.push_back(strand_desc);
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

    void Scene::update(float dt)
    {
    }
}