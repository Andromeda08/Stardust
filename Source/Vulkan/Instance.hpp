#pragma once

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "../Utility/Macro.hpp"
#include "../Window.hpp"

class Instance {
public:
    NON_COPIABLE(Instance)

    explicit Instance(const Window& window);

    vk::Instance handle() const { return mInstance; }

    const Window& window() const { return mWindow; }

    const std::vector<std::string>& instanceLayers() const { return mInstanceLayers; }

    const std::vector<std::string>& instanceExtensions() const { return mInstanceExtensions; }

    const std::vector<vk::PhysicalDevice>& physicalDevices() const { return mPhysicalDevices; }

private:
    /**
     * @brief Enumerates all Discrete and Integrated GPU Physical Devices and
     * stores them in a class member variable.
     */
    void enumeratePhysicalDevices();

private:
    vk::Instance mInstance;

    const Window& mWindow;

    // Lists of validation layers and extensions used by the vulkan instance
    std::vector<std::string> mInstanceLayers;
    std::vector<std::string> mInstanceExtensions;

    // List of detected physical devices
    std::vector<vk::PhysicalDevice> mPhysicalDevices;
};