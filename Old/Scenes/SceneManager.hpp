#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <Scenes/Scene.hpp>

namespace sd
{
    class SceneManager
    {
        struct NamedScene
        {
            std::unique_ptr<Scene> scene;
            std::string name;
        };

    public:
        SceneManager();;

        void register_keybinds(GLFWwindow* window);

        void change_scene(int direction);

        void add_scene(std::function<std::unique_ptr<Scene>()> const& generator, std::string const& name = "", bool make_active = false);

        void render_active_scene(uint32_t frame, vk::CommandBuffer cmd);

    private:
        uint32_t m_active_scene {0};
        std::vector<NamedScene> m_scenes;
        std::unique_ptr<Clock> m_clock;
    };
}