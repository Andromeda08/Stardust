#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

Camera::Camera(vk::Extent2D extent, glm::vec3 eye, float fov, float near, float far)
{
    m_extent = extent;
    m_eye    = eye;
    m_fov    = fov;
    m_near   = near;
    m_far    = far;
}

glm::vec3 Camera::eye() const
{
    return m_eye;
}

glm::mat4 Camera::view() const
{
    return glm::lookAt(m_eye, m_eye + m_orientation, m_up);
}

glm::mat4 Camera::projection() const
{
    return glm::perspective(glm::radians(m_fov), (float) m_extent.width / (float) m_extent.height, m_near, m_far);
}

void Camera::use_inputs(GLFWwindow *t_window)
{
    // WASD movement
    if (glfwGetKey(t_window, GLFW_KEY_W) == GLFW_PRESS)
    {
        m_eye += m_speed * m_orientation;
    }
    if (glfwGetKey(t_window, GLFW_KEY_A) == GLFW_PRESS)
    {
        m_eye += m_speed * -glm::normalize(glm::cross(m_orientation, m_up));
    }
    if (glfwGetKey(t_window, GLFW_KEY_S) == GLFW_PRESS)
    {
        m_eye += m_speed * -m_orientation;
    }
    if (glfwGetKey(t_window, GLFW_KEY_D) == GLFW_PRESS)
    {
        m_eye += m_speed * glm::normalize(glm::cross(m_orientation, m_up));
    }

    // Move up & down
    if (glfwGetKey(t_window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        m_eye += (m_speed / 2.0f) * m_up;
    }
    if (glfwGetKey(t_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        m_eye -= (m_speed / 2.0f) * m_up;
    }

    // Exit on ESC
    if (glfwGetKey(t_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(t_window, true);
    }


    if (glfwGetMouseButton(t_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        glfwSetInputMode(t_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        if (m_first_click)
        {
            glfwSetCursorPos(t_window, (m_extent.width / 2), (m_extent.height / 2));
            m_first_click = false;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(t_window, &mouseX, &mouseY);

        float rotX = m_sensitivity * (float)(mouseY - (m_extent.height / 2)) / m_extent.height;
        float rotY = m_sensitivity * (float)(mouseX - (m_extent.width / 2)) / m_extent.width;

        glm::vec3 newOrientation = glm::rotate(m_orientation, glm::radians(-rotX), glm::normalize(glm::cross(m_orientation, m_up)));

        if (abs(glm::angle(newOrientation, m_up) - glm::radians(90.0f)) <= glm::radians(85.0f))
        {
            m_orientation = newOrientation;
        }

        m_orientation = glm::rotate(m_orientation, glm::radians(-rotY), m_up);
        glfwSetCursorPos(t_window, (m_extent.width / 2), (m_extent.height / 2));
    }
    else if (glfwGetMouseButton(t_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        glfwSetInputMode(t_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_first_click = true;
    }
}
