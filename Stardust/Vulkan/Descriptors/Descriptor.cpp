#include "Descriptor.hpp"

namespace sdvk
{

    Descriptor::Descriptor(const std::vector<vk::DescriptorSetLayoutBinding>& bindings,
                           const vk::DescriptorSetLayout& layout, const vk::Device& device, uint32_t count)
    : m_device(device)
    , m_bindings(bindings)
    , m_layout(layout)
    {
        vk::Result result;

        std::vector<vk::DescriptorPoolSize> pool_sizes;
        for (const auto& b : m_bindings)
        {
            pool_sizes.emplace_back( b.descriptorType, count );
        }

        vk::DescriptorPoolCreateInfo create_info;
        create_info.setMaxSets(count);
        create_info.setPoolSizeCount(pool_sizes.size());
        create_info.setPPoolSizes(pool_sizes.data());
        result = m_device.createDescriptorPool(&create_info, nullptr, &m_pool);

        std::vector<vk::DescriptorSetLayout> layouts(count, m_layout);
        vk::DescriptorSetAllocateInfo allocate_info;
        allocate_info.setDescriptorPool(m_pool);
        allocate_info.setDescriptorSetCount(count);
        allocate_info.setPSetLayouts(layouts.data());

        m_sets.resize(count);
        result = m_device.allocateDescriptorSets(&allocate_info, m_sets.data());
    }

    const vk::DescriptorSet& Descriptor::set(uint32_t id) const
    {
        if (id > m_sets.size())
        {
            throw std::out_of_range("Index" + std::to_string(id) + " out of range for descriptor set.");
        }

        return m_sets[id];
    }

    const vk::DescriptorSet& Descriptor::operator[](uint32_t id) const
    {
        return set(id);
    }

    const vk::DescriptorSetLayout& Descriptor::layout() const
    {
        return m_layout;
    }
}