#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>

class DescriptorSets
{
public:
    NON_COPIABLE(DescriptorSets)

    DescriptorSets(const std::vector<vk::DescriptorSetLayoutBinding>& layout_bindings,
                   vk::DescriptorSetLayout layout,
                   const Device& device);

    void update_descriptor_set(uint32_t index,
                               uint32_t binding,
                               const vk::DescriptorBufferInfo& buffer_info);

    void update_descriptor_set(uint32_t index,
                               uint32_t binding,
                               const vk::DescriptorImageInfo& image_info);

    const vk::DescriptorSet& operator[](size_t i) const { return get_set(i); }

    const vk::DescriptorSet& get_set(size_t i) const { return m_sets[i]; }

    vk::DescriptorSetLayout layout() const { return m_layout; }

private:
    vk::DescriptorPool m_pool;

    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;

    vk::DescriptorSetLayout m_layout;

    std::vector<vk::DescriptorSet> m_sets;

    const Device& m_device;
};
