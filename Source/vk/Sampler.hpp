#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Device.hpp>

namespace re
{
    class Sampler
    {
    public:
        explicit Sampler(const Device& device);

        static vk::DescriptorSetLayoutBinding make_binding(uint32_t binding);

        const vk::Sampler& sampler() const { return m_sampler; }

    private:
        vk::Sampler m_sampler;
    };
}