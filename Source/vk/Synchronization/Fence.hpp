#pragma once

#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>

class Fence {
public:
    NON_COPIABLE(Fence)

    explicit Fence(const Device& device);

    Fence(Fence const&&) noexcept;

    const vk::Fence& handle() const { return mFence; }

    const Device& device() const { return mDevice; }

private:
    vk::Fence mFence;

    const Device& mDevice;
};
