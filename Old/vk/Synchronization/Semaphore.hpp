#pragma once

#include <vulkan/vulkan.hpp>
#include <vk/Device/Device.hpp>

class Semaphore {
public:
    NON_COPIABLE(Semaphore)

    explicit Semaphore(const Device& device);

    Semaphore(Semaphore const&&) noexcept;

    const vk::Semaphore& handle() const { return mSemaphore; }

    const Device& device() const { return mDevice; }

private:
    vk::Semaphore mSemaphore;

    const Device& mDevice;
};
