#include "Device.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

Device::Device(const Surface& surface, vk::PhysicalDevice physicalDevice, const std::vector<const char*>& requiredExtensions)
: mPhysicalDevice(physicalDevice)
, mSurface(surface)
{
    checkRequiredDeviceExtensions(mPhysicalDevice, requiredExtensions);

    // Find queues
    const auto queueFamilies   = mPhysicalDevice.getQueueFamilyProperties();
    const auto graphicsFamily = findQueue(queueFamilies, vk::QueueFlagBits::eGraphics, {});
    const auto computeFamily  = findQueue(queueFamilies, vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics);

    // Get index using iterator dark magic
    mGraphicsFamilyIdx = static_cast<uint32_t>(graphicsFamily - std::begin(queueFamilies));
    mComputeFamilyIdx  = static_cast<uint32_t>(computeFamily  - std::begin(queueFamilies));

    // Get present family index
    uint32_t presentIndex = 0;
    for (const vk::QueueFamilyProperties& props : queueFamilies) {
        if (mPhysicalDevice.getSurfaceSupportKHR(presentIndex, mSurface.handle())) {
            mPresentFamilyIdx = presentIndex;
        }
        presentIndex++;
    }

    // => set of unique queues
    const std::set<uint32_t> uniqueQueueFamilies = { mGraphicsFamilyIdx, mComputeFamilyIdx, mPresentFamilyIdx };

    // Create queues
    float queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (auto index : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo createInfo;

        createInfo.setQueueFamilyIndex(index);
        createInfo.setQueueCount(1);
        createInfo.setPQueuePriorities(&queuePriority);

        queueCreateInfos.push_back(createInfo);
    }

    // Validation layers
    std::vector<const char*> validationLayers;
    for (const auto& layer : mSurface.instance().instanceLayers())
    {
        validationLayers.push_back(layer.c_str());
    }

    // Device features
    vk::PhysicalDeviceFeatures deviceFeatures {};

    // Create Device
    vk::DeviceCreateInfo createInfo;

    createInfo.setPEnabledFeatures(&deviceFeatures);
    createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
    createInfo.setPpEnabledLayerNames(validationLayers.data());
    createInfo.setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()));
    createInfo.setPpEnabledExtensionNames(requiredExtensions.data());
    createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()));
    createInfo.setPQueueCreateInfos(queueCreateInfos.data());
    createInfo.setPNext(nullptr);

    auto result = mPhysicalDevice.createDevice(&createInfo, nullptr, &mDevice);

    mDevice.getQueue(mGraphicsFamilyIdx, 0, &mGraphicsQueue);
    mDevice.getQueue(mPresentFamilyIdx, 0, &mPresentQueue);
    mDevice.getQueue(mComputeFamilyIdx, 0, &mComputeQueue);
}

void Device::waitIdle() const
{
    mDevice.waitIdle();
}

uint32_t Device::findMemoryType(uint32_t filter, vk::MemoryPropertyFlags flags) const {
    auto props = mPhysicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
    {
        if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

void Device::checkRequiredDeviceExtensions(vk::PhysicalDevice physicalDevice, const std::vector<const char*>& requiredExtensions) {
    const auto extensions = physicalDevice.enumerateDeviceExtensionProperties();

    // Create set from passed requiredExtensions and remove each extension if present.
    // When set is empty all required extensions are present.
    std::set<std::string> _requiredExtensions(std::begin(requiredExtensions), std::end(requiredExtensions));
    for (const auto& extension : extensions) {
        _requiredExtensions.erase(extension.extensionName);
    }

    if (!_requiredExtensions.empty())
    {
        throw std::runtime_error("There are missing device extensions!");
    }
}

std::vector<vk::QueueFamilyProperties>::const_iterator Device::findQueue(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
                                                                         vk::QueueFlagBits required,
                                                                         vk::QueueFlagBits excluded)
{
    const auto family = std::find_if(
        std::begin(queueFamilies),
        std::end(queueFamilies),
        [required, excluded](const vk::QueueFamilyProperties& props) {
            return (props.queueCount > 0)
                   && (props.queueFlags & required)
                   && !(props.queueFlags & excluded);
        });

    if (family == queueFamilies.end())
    {
        throw std::runtime_error("Failed to find some queue family.");
    }

    return family;
}
