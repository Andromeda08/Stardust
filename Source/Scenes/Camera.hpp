#pragma once

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

class Camera
{
public:
    Camera(vk::Extent2D extent, glm::vec3 eye, float fov = 75.0f, float near = 0.1f, float far = 10000.0f);

    glm::vec3 eye() const;

    glm::mat4 view() const;

    glm::mat4 projection() const;

    void use_inputs(GLFWwindow* t_window);

private:
    vk::Extent2D m_extent = { 1280, 720 };
    glm::vec3    m_eye  = { 5.0f, 0.0f, 5.0f };
    glm::vec3    m_up   = { 0, 1, 0 };
    glm::vec3    m_orientation = { 0, 0, -1 };

    float m_near = 0.1f;
    float m_far  = 10000.0f;

    float m_fov = 75.0f;
    float m_speed = 0.025f;
    float m_sensitivity = 50.0f;

    bool m_first_click = false;
};