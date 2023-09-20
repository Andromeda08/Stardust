#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include "ResourceRole.hpp"
#include "ResourceType.hpp"

namespace Nebula::RenderGraph
{
    struct ResourceSpecification
    {
        std::string  name { "Unknown Resource" };
        ResourceRole role { ResourceRole::eUnknown };
        ResourceType type { ResourceType::eUnknown };

        // Image Related
        vk::Format format { vk::Format::eR32G32B32A32Sfloat };
        vk::ImageAspectFlags aspect_flags { vk::ImageAspectFlagBits::eColor };
        vk::Extent2D extent { 1920, 1080 };
        vk::MemoryPropertyFlags memory_flags { vk::MemoryPropertyFlagBits::eDeviceLocal };
        vk::SampleCountFlagBits sample_count { vk::SampleCountFlagBits::e1 };
        vk::ImageTiling tiling { vk::ImageTiling::eOptimal };
        vk::ImageUsageFlags usage_flags { vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage };
    };
}