#include "Semaphore.hpp"

Semaphore::Semaphore(const Device& device)
: mDevice(device)
{
    vk::SemaphoreCreateInfo createInfo = {};
    auto result = mDevice.handle().createSemaphore(&createInfo, nullptr, &mSemaphore);
}

Semaphore::Semaphore(const Semaphore&& s) noexcept
: mDevice(s.mDevice)
, mSemaphore(s.mSemaphore)
{
}
