#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Descriptors/Descriptor.hpp>

namespace sdvk
{
    struct DescriptorWrites
    {
        DescriptorWrites(const vk::Device& device, const Descriptor& sets) : _device(device), _sets(sets) {}

        DescriptorWrites& acceleration_structure(uint32_t set, uint32_t binding, const vk::WriteDescriptorSetAccelerationStructureKHR& pNext)
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(binding);
            write.setDstSet(_sets[set]);
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
            write.setDstArrayElement(0);
            write.setPNext(&pNext);

            _writes.push_back(write);
            return *this;
        }

        DescriptorWrites& storage_buffer(uint32_t set, uint32_t binding, const vk::DescriptorBufferInfo& buffer_info)
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(binding);
            write.setDstSet(_sets[set]);
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageBuffer);
            write.setPBufferInfo(&buffer_info);
            write.setDstArrayElement(0);

            _writes.push_back(write);
            return *this;
        }

        DescriptorWrites& storage_image(uint32_t set, uint32_t binding, const vk::DescriptorImageInfo& image_info)
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(1);
            write.setDstSet(_sets[set]);
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eStorageImage);
            write.setPImageInfo(&image_info);
            write.setDstArrayElement(0);

            _writes.push_back(write);
            return *this;
        }

        DescriptorWrites& uniform_buffer(uint32_t set, uint32_t binding, const vk::DescriptorBufferInfo& buffer_info)
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(binding);
            write.setDstSet(_sets[set]);
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eUniformBuffer);
            write.setPBufferInfo(&buffer_info);
            write.setDstArrayElement(0);

            _writes.push_back(write);
            return *this;
        }

        DescriptorWrites& combined_image_sampler(uint32_t set, uint32_t binding, const vk::DescriptorImageInfo& image_info)
        {
            vk::WriteDescriptorSet write;
            write.setDstBinding(binding);
            write.setDstSet(_sets[set]);
            write.setDescriptorCount(1);
            write.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            write.setPImageInfo(&image_info);
            write.setDstArrayElement(0);

            _writes.push_back(write);
            return *this;
        }

        void commit()
        {
            _device.updateDescriptorSets(_writes.size(),_writes.data(), 0, nullptr);
        }

        template <typename T>
        static vk::DescriptorBufferInfo buffer_info(const vk::Buffer& buffer, vk::DeviceSize offset = 0)
        {
            vk::DescriptorBufferInfo info;
            info.setRange(sizeof(T));
            info.setOffset(offset);
            info.setBuffer(buffer);
            return info;
        }

    private:
        std::vector<vk::WriteDescriptorSet> _writes;

        const vk::Device& _device;
        const Descriptor& _sets;
    };
}