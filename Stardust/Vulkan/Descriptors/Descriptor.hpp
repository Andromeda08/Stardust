#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    class Descriptor
    {
    public:
        Descriptor(Descriptor const&) = delete;
        Descriptor& operator=(Descriptor const&) = delete;

        Descriptor(std::vector<vk::DescriptorSetLayoutBinding> const& bindings,
                   vk::DescriptorSetLayout const& layout, vk::Device const& device, uint32_t count);

        const vk::DescriptorSet& set(uint32_t id) const;

        const vk::DescriptorSet& operator[](uint32_t id) const;

        const vk::DescriptorSetLayout& layout() const;

    private:
        vk::DescriptorPool m_pool;
        std::vector<vk::DescriptorSet> m_sets;
        std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
        vk::DescriptorSetLayout m_layout;

        const vk::Device& m_device;
    };
}
