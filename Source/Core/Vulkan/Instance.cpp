#include "Instance.hpp"

Instance::Instance(const Window& window)
: mWindow(window)
{
    // Enumerate and check available instance layers.
    auto layers = vk::enumerateInstanceLayerProperties();

    std::set<std::string> requiredLayers { "VK_LAYER_KHRONOS_validation" };

    std::vector<const char*> foundLayers;
    for (const auto& layer : layers)
    {
        if (requiredLayers.count(layer.layerName.data()))
        {
            foundLayers.push_back(layer.layerName);
        }
    }

    // Enumerate and check available instance extensions.
    auto extensions = vk::enumerateInstanceExtensionProperties();

    std::set<std::string> requiredExtensions { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    for (const auto glfwExtension : Window::get_instance_extensions())
    {
        requiredExtensions.insert(glfwExtension);
    }

    std::vector<const char*> foundExtensions;
    for (const auto& extension : extensions)
    {
        if (requiredExtensions.count(extension.extensionName.data()))
        {
            foundExtensions.push_back(extension.extensionName);
        }
    }

    // Save layers and extensions
    std::copy(foundLayers.begin(), foundLayers.end(), std::back_inserter(mInstanceLayers));
    std::copy(foundExtensions.begin(), foundExtensions.end(), std::back_inserter(mInstanceExtensions));

    // Create Vulkan instance
    vk::ApplicationInfo appInfo;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.setEnabledLayerCount(foundLayers.size());
    createInfo.setPpEnabledLayerNames(foundLayers.data());
    createInfo.setEnabledExtensionCount(foundExtensions.size());
    createInfo.setPpEnabledExtensionNames(foundExtensions.data());

    auto result = vk::createInstance(&createInfo, nullptr, &mInstance);

    enumeratePhysicalDevices();
}

void Instance::enumeratePhysicalDevices()
{
    auto physicalDevices = mInstance.enumeratePhysicalDevices();

    std::vector<vk::PhysicalDevice> foundDevices;
    for (const auto& pd : physicalDevices)
    {
        vk::PhysicalDeviceProperties props = pd.getProperties();
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        {
            foundDevices.push_back(pd);
        }
    }

    mPhysicalDevices = foundDevices;
}
