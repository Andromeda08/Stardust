#pragma once

#include <memory>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "Instance.hpp"
#include "Surface.hpp"
#include "../Utility/Macro.hpp"

class Device {
public:
    NON_COPIABLE(Device)

    Device(const Instance& instance,
           const Surface& surface,
           vk::PhysicalDevice physicalDevice,
           const std::vector<const char*>& requiredExtensions);

    void waitIdle() const;

    uint32_t findMemoryType(uint32_t filter, vk::MemoryPropertyFlags flags) const;

    vk::Device handle() const { return mDevice; }

    const vk::PhysicalDevice& physicalDevice() const { return mPhysicalDevice; }

    const vk::DispatchLoaderDynamic& dispatch() const { return mDispatch; }

    const Surface& surface() const { return mSurface; }

    #pragma region Queue_indices_and_handles

    uint32_t  graphics_index() const { return mGraphicsFamilyIdx; }
    vk::Queue graphics_queue() const { return mGraphicsQueue; }

    uint32_t  present_index() const { return mPresentFamilyIdx; }
    vk::Queue present_queue() const { return mPresentQueue; }

    uint32_t  compute_index() const { return mComputeFamilyIdx; }
    vk::Queue compute_queue() const { return mComputeQueue; }

    #pragma endregion

private:
    /**
     * @brief Checks if all required extensions are supported by the device.
     */
    static void checkRequiredDeviceExtensions(vk::PhysicalDevice physicalDevice,
                                              const std::vector<const char*>& requiredExtensions);

    /**
     * @brief Searches for a queue with the appropriate flags
     * @param required Required queue flag bits
     * @param excluded Excluded queue flag bits
     */
    static std::vector<vk::QueueFamilyProperties>::const_iterator findQueue(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
                                                                     vk::QueueFlagBits required,
                                                                     vk::QueueFlagBits excluded);

private:
    vk::PhysicalDevice mPhysicalDevice;

    vk::Device mDevice;

    vk::DispatchLoaderDynamic mDispatch;

    const Surface& mSurface;

    uint32_t  mGraphicsFamilyIdx {};
    vk::Queue mGraphicsQueue {};

    uint32_t  mPresentFamilyIdx {};
    vk::Queue mPresentQueue {};

    uint32_t  mComputeFamilyIdx {};
    vk::Queue mComputeQueue {};
};