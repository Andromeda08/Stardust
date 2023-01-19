#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct Queue
    {
        uint32_t  index { 0 };
        vk::Queue queue { nullptr };
    };
}