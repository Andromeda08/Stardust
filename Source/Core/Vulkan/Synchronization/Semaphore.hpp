#pragma once

#include <vulkan/vulkan.hpp>
#include "../Device.hpp"
#include "../../Macro.hpp"

class Semaphore {
public:
    NON_COPIABLE(Semaphore)

    explicit Semaphore(const Device& device);

    Semaphore(Semaphore const&&) noexcept;

    vk::Semaphore handle() const { return mSemaphore; }

    const Device& device() const { return mDevice; }

private:
    vk::Semaphore mSemaphore;

    const Device& mDevice;
};
