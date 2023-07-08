#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct Queue
    {
        uint32_t  index { 0 };
        vk::Queue queue { nullptr };
    };
}