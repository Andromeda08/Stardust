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

    private:
        void add_defaults();

        void create_acceleration_structure();

        void default_init();

    private:
        std::shared_ptr<Camera> m_camera;
        std::vector<Object>     m_objects;
        std::vector<Light>      m_lights;

        std::map<std::string, std::shared_ptr<sdvk::Mesh>> m_meshes;
        std::shared_ptr<sdvk::Tlas> m_acceleration_structure;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context& m_context;
    };
}