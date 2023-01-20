#pragma once

#include <vulkan/vulkan.hpp>
#include <Vulkan/Utils.hpp>

namespace sdvk
{
    struct SamplerBuilder
    {
        explicit SamplerBuilder()
        {
            create_info.setMagFilter(vk::Filter::eLinear);
            create_info.setMinFilter(vk::Filter::eLinear);
            create_info.setAddressModeU(vk::SamplerAddressMode::eRepeat);
            create_info.setAddressModeV(vk::SamplerAddressMode::eRepeat);
            create_info.setAddressModeW(vk::SamplerAddressMode::eRepeat);
            create_info.setAnisotropyEnable(false);
            create_info.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
            create_info.setUnnormalizedCoordinates(false);
            create_info.setCompareEnable(false);
            create_info.setCompareOp(vk::CompareOp::eAlways);
            create_info.setMipmapMode(vk::SamplerMipmapMode::eLinear);
            create_info.setMipLodBias(0.0f);
            create_info.setMinLod(0.0f);
            create_info.setMaxLod(0.0f);
        }

        SamplerBuilder& with_anisotropy(float max_anisotropy)
        {
            create_info.setAnisotropyEnable(true);
            create_info.setMaxAnisotropy(max_anisotropy);
            return *this;
        }

        SamplerBuilder& with_create_info(vk::SamplerCreateInfo const& r_create_info)
        {
            create_info = r_create_info;
            return *this;
        }

        vk::Sampler create(vk::Device const& device)
        {
            vk::Sampler result;
            auto r = device.createSampler(&create_info, nullptr, &result);
            sdvk::util::name_vk_object("Sampler", (uint64_t) static_cast<VkSampler>(result), vk::ObjectType::eSampler, device);
            return result;
        }

    private:
        vk::SamplerCreateInfo create_info;
    };
}