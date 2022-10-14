#include "DescriptorSets.hpp"

DescriptorSets::DescriptorSets(const std::vector<vk::DescriptorSetLayoutBinding>& layout_bindings,
                               vk::DescriptorSetLayout layout,
                               const Device& device)
: m_device(device)
, m_bindings(layout_bindings)
{
    vk::Result result;
    vk::Device h_device = m_device.handle();

    std::vector<vk::DescriptorPoolSize> pool_sizes;
    for (const auto& b : m_bindings)
    {
        vk::DescriptorPoolSize ps;
        ps.setType(b.descriptorType);
        ps.setDescriptorCount(2);
        pool_sizes.push_back(ps);
    }

    vk::DescriptorPoolCreateInfo create_info;
    create_info.setMaxSets(2);
    create_info.setPoolSizeCount(pool_sizes.size());
    create_info.setPPoolSizes(pool_sizes.data());
    result = h_device.createDescriptorPool(&create_info, nullptr, &m_pool);

    std::vector<vk::DescriptorSetLayout> layouts(2, layout);
    vk::DescriptorSetAllocateInfo alloc_info;
    alloc_info.setDescriptorPool(m_pool);
    alloc_info.setDescriptorSetCount(2);
    alloc_info.setPSetLayouts(layouts.data());

    m_sets.resize(2);
    result = h_device.allocateDescriptorSets(&alloc_info, m_sets.data());
}

void DescriptorSets::update_descriptor_set(uint32_t index,
                                           uint32_t binding,
                                           const vk::DescriptorBufferInfo& buffer_info)
{
    std::vector<vk::WriteDescriptorSet> writes;

    vk::WriteDescriptorSet write;
    write.setDstBinding(binding);
    write.setDstSet(m_sets[index]);
    write.setDescriptorCount(1);
    write.setDescriptorType(m_bindings[binding].descriptorType);
    write.setPBufferInfo(&buffer_info);
    write.setDstArrayElement(0);
    write.setPNext(nullptr);
    writes.push_back(write);

    m_device.handle().updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
