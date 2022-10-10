#include "Fence.hpp"

Fence::Fence(const Device& device)
: mDevice(device)
{
    vk::FenceCreateInfo createInfo = {};
    auto result = mDevice.handle().createFence(&createInfo, nullptr, &mFence);
}

Fence::Fence(const Fence&& f) noexcept
: mDevice(f.mDevice)
, mFence(f.mFence)
{
}