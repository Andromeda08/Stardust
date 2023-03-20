#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace sd::rg
{
    struct Resource
    {
        enum class Type { eBuffer, eImage } type;
    };

    struct Pass
    {
        std::vector<Resource> resources;
        std::vector<std::pair<std::string, std::string>> resolve;
        std::function<void(vk::CommandBuffer&)> execute;
    };
}