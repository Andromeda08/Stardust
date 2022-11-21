#include "Device.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>

Device::Device(const Instance& instance,
               const Surface& surface,
               vk::PhysicalDevice physicalDevice,
               const std::vector<const char*>& requiredExtensions)
: mPhysicalDevice(physicalDevice)
, mSurface(surface)
{
    checkRequiredDeviceExtensions(mPhysicalDevice, requiredExtensions);

    // Find queues
    const auto queueFamilies   = mPhysicalDevice.getQueueFamilyProperties();
    const auto graphicsFamily = findQueue(queueFamilies, vk::QueueFlagBits::eGraphics, {});
#if !defined(__APPLE__)
    const auto computeFamily  = findQueue(queueFamilies, vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics);
#endif

    // Get index using iterator dark magic
    mGraphicsFamilyIdx = static_cast<uint32_t>(graphicsFamily - std::begin(queueFamilies));
#if !defined(__APPLE__)
    mComputeFamilyIdx  = static_cast<uint32_t>(computeFamily  - std::begin(queueFamilies));
#endif

    // Get present family index
    uint32_t presentIndex = 0;
    for (const vk::QueueFamilyProperties& props : queueFamilies) {
        if (mPhysicalDevice.getSurfaceSupportKHR(presentIndex, mSurface.handle())) {
            mPresentFamilyIdx = presentIndex;
        }
        presentIndex++;
    }

    // => set of unique queues
    std::set<uint32_t> uniqueQueueFamilies = { mGraphicsFamilyIdx, mPresentFamilyIdx };

#if !defined(__APPLE__)
    uniqueQueueFamilies.insert(mComputeFamilyIdx);
#endif

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
    deviceFeatures.setSamplerAnisotropy(true);
    deviceFeatures.setShaderInt64(true);

    vk::PhysicalDeviceSynchronization2FeaturesKHR s2_features;
    s2_features.setSynchronization2(true);

    vk::PhysicalDeviceMaintenance4FeaturesKHR m4_features;
    m4_features.setMaintenance4(true);
    m4_features.setPNext(&s2_features);

    vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeaturesExt;
    descriptorIndexingFeaturesExt.setRuntimeDescriptorArray(true);
    descriptorIndexingFeaturesExt.setPNext(&m4_features);

    vk::PhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeaturesExt;
    bufferDeviceAddressFeaturesExt.setBufferDeviceAddress(true);
    bufferDeviceAddressFeaturesExt.setPNext(&descriptorIndexingFeaturesExt);

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR as_features;
    as_features.setAccelerationStructure(true);
    as_features.setPNext(&bufferDeviceAddressFeaturesExt);

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rt_features;
    rt_features.setRayTracingPipeline(true);
    rt_features.setPNext(&as_features);

    // Create Device
    vk::DeviceCreateInfo createInfo;

    createInfo.setPEnabledFeatures(&deviceFeatures);
    createInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
    createInfo.setPpEnabledLayerNames(validationLayers.data());
    createInfo.setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()));
    createInfo.setPpEnabledExtensionNames(requiredExtensions.data());
    createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()));
    createInfo.setPQueueCreateInfos(queueCreateInfos.data());
    createInfo.setPNext(&rt_features);

    auto result = mPhysicalDevice.createDevice(&createInfo, nullptr, &mDevice);

    mDevice.getQueue(mGraphicsFamilyIdx, 0, &mGraphicsQueue);
    mDevice.getQueue(mPresentFamilyIdx, 0, &mPresentQueue);

#if !defined(__APPLE__)
    mDevice.getQueue(mComputeFamilyIdx, 0, &mComputeQueue);
#endif

    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    mDispatch = vk::DispatchLoaderDynamic(instance.handle(), vkGetInstanceProcAddr, mDevice);
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
