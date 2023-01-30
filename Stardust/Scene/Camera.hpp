#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <Resources/CameraUniformData.hpp>

namespace sd
{
    class Camera
    {
    public:
        Camera(glm::ivec2 size, glm::vec3 eye, float h_fov = 75.0f, float near = 0.1f, float far = 10000.0f);

        const glm::vec3& eye() const { return m_eye; }

        glm::mat4 view() const;

        glm::mat4 projection() const;

        CameraUniformData uniform_data() const;

        void register_keys(GLFWwindow* p_window);

    private:
        glm::ivec2 m_size;
        glm::vec3  m_eye;
        glm::vec3  m_up = {0, 1, 0};
        glm::vec3  m_orientation = {0, 0, -1};

        float m_near;
        float m_far;
        float m_fov;

        float m_speed {0.025f};
        float m_sensitivity {50.0f};

        bool m_click {false};
    };
}