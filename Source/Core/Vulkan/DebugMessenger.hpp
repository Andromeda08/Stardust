#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Debug
{
    static VkDebugUtilsMessageSeverityFlagsEXT vk_debug_msg_severities()
    {
        return VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
             | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    }

    static VkDebugUtilsMessageTypeFlagsEXT vk_debug_msg_types()
    {
        return VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cerr << pCallbackData->pMessage << std::endl;
        }

        return VK_FALSE;
    }

    static VkResult create_vk_debug_msgr_ext(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void destroy_vk_debug_msgr_ext(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }
}