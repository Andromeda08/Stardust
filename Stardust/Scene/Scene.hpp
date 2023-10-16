#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <Scene/Camera.hpp>
#include <Scene/Light.hpp>
#include <Scene/Object.hpp>

namespace sdvk
{
    class CommandBuffers;
    class Context;
    class Mesh;
    struct Tlas;
}

namespace sd
{
    class Camera;
    class Window;

    struct ObjDescription
    {
        uint64_t  vertex_buffer;
        uint64_t  index_buffer;
    };

    class Scene
    {
    public:
        Scene(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context);

        Scene(const std::function<void()>& init, const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context);

        virtual void key_handler(const Window& window);

        virtual void mouse_handler(const Window& window);

    public:
        const std::shared_ptr<Camera>& camera() const { return m_camera; }

        const std::vector<Object>& objects() const { return m_objects; }

        const std::shared_ptr<sdvk::Tlas>& acceleration_structure() const { return m_acceleration_structure; }

        const std::shared_ptr<sdvk::Buffer>& object_descriptions() const { return m_obj_desc_buffer; }

        const std::string& name() const { return m_name; }

    private:
        void add_defaults();

        void create_acceleration_structure();

        void create_object_description_buffer(const sdvk::CommandBuffers& command_buffers);

        void default_init();

    private:
        std::shared_ptr<Camera>     m_camera;
        std::vector<Object>         m_objects;
        std::vector<Light>          m_lights;
        std::vector<ObjDescription> m_obj_descriptions;

        std::map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;
        std::shared_ptr<sdvk::Tlas> m_acceleration_structure;
        std::shared_ptr<sdvk::Buffer> m_obj_desc_buffer;

        const std::string m_name = "Unnamed Scene";
        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context& m_context;
    };
}