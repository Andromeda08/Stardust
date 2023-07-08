#pragma once

#include <memory>
#include <string>
#include "Transform.hpp"
#include <Vulkan/Rendering/Mesh.hpp>
#include <glm/glm.hpp>

namespace sd
{
    struct Object
    {
        Transform transform;
        std::shared_ptr<sdvk::Mesh> mesh;
        glm::vec4 color { 0.5f, 0.5f, 0.5f, 1.0f };
        std::string name = "object";

        uint32_t rt_hit_group { 0 };
        uint32_t rt_mask { 0xff };

        struct PushConstantData
        {
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec4 color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        };
    };
}