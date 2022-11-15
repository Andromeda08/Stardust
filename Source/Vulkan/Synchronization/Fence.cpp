#include "Fence.hpp"

Fence::Fence(const Device& device)
: mDevice(device)
{
    vk::FenceCreateInfo createInfo = {};
    createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    auto result = mDevice.handle().createFence(&createInfo, nullptr, &mFence);
}

Fence::Fence(const Fence&& f) noexcept
: mDevice(f.mDevice)
, mFence(f.mFence)
{
}