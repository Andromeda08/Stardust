#include "SceneManager.hpp"

#include <sstream>

sd::SceneManager::SceneManager()
{
    m_clock = std::make_unique<Clock>();
    m_clock->start();
}

void sd::SceneManager::register_keybinds(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        change_scene(-1);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        change_scene(1);
    }

    m_scenes[m_active_scene].scene->register_keybinds(window);
}

void sd::SceneManager::change_scene(int direction)
{
    if (m_clock->ms() >= 750)
    {
        auto previous = m_active_scene;
        m_active_scene = static_cast<uint32_t>(std::clamp((int) m_active_scene + direction, 0, (int) m_scenes.size() - 1));
        m_clock->start();

        if (previous != m_active_scene)
        {
            std::stringstream msg;
            msg << "Scenes : ";
            for (int i = 0; i < m_scenes.size(); i++)
            {
                if (i != 0) msg << " ";
                msg << ((m_active_scene == i) ? "[" + m_scenes[i].name + "]" : m_scenes[i].name);
            }
            std::cout << msg.str() << std::endl;
        }
    }
}

void sd::SceneManager::add_scene(const std::function<std::unique_ptr<Scene>()> &generator, const std::string &name, bool make_active)
{
    m_scenes.push_back({ generator(), (name.empty()) ? "Scene [" + std::to_string(m_scenes.size()) + "]" : name });

    if (make_active)
    {
        m_active_scene = m_scenes.size() - 1;
    }
}

void sd::SceneManager::render_active_scene(uint32_t frame, vk::CommandBuffer cmd)
{
    try
    {
        m_scenes[m_active_scene].scene->trace_rays(frame, cmd);
    }
    catch (std::runtime_error& ex)
    {
        m_scenes[m_active_scene].scene->rasterize(frame, cmd);
    }
}
