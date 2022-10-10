#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Macro.hpp"

class Fence {
public:
    NON_COPIABLE(Fence)

    explicit Fence(const Device& device);

    Fence(Fence const&&) noexcept;

    vk::Fence handle() const { return mFence; }

    const Device& device() const { return mDevice; }

private:
    vk::Fence mFence;

    const Device& mDevice;
};
