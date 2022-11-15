#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Device.hpp>

namespace re
{
    class Sampler
    {
    public:
        explicit Sampler(const Device& device)
        {
            auto props = device.physicalDevice().getProperties();

            vk::SamplerCreateInfo create_info;
            create_info.setMagFilter(vk::Filter::eLinear);
            create_info.setMagFilter(vk::Filter::eLinear);
            create_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
            create_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
            create_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
            create_info.setAnisotropyEnable(true);
            create_info.setMaxAnisotropy(props.limits.maxSamplerAnisotropy);
            create_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
            create_info.setUnnormalizedCoordinates(false);
            create_info.setCompareEnable(false);
            create_info.setCompareOp(vk::CompareOp::eAlways);
            create_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
            create_info.setMipLodBias(0.0f);
            create_info.setMinLod(0.0f);
            create_info.setMaxLod(0.0f);
            auto result = device.handle().createSampler(&create_info, nullptr, &m_sampler, device.dispatch());
        }

        static vk::DescriptorSetLayoutBinding make_binding(uint32_t binding)
        {
            vk::DescriptorSetLayoutBinding b;
            b.setBinding(binding);
            b.setDescriptorCount(1);
            b.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
            b.setStageFlags(vk::ShaderStageFlagBits::eFragment);
            return b;
        }

        const vk::Sampler& sampler() const { return m_sampler; }

    private:
        vk::Sampler m_sampler;
    };
}