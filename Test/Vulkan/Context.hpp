#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "ContextOptions.hpp"
#include "DeviceFeatures.hpp"
#include "Queues.hpp"

namespace sdvk
{
    class Context
    {
    public:
        Context(Context const&) = delete;
        Context& operator=(Context const&) = delete;

        explicit Context(ContextOptions const& options);

        const vk::Device& device() const { return m_device; }
        const vk::PhysicalDevice& physical_device() const { return m_physical_device; }

        const VkSurfaceKHR& surface() const { return m_surface; }

        const Queue& q_compute()  const { return m_compute_queue;  }
        const Queue& q_graphics() const { return m_graphics_queue; }
        const Queue& q_present()  const { return m_present_queue;  }

        bool is_debug() const { return m_debug_messenger; }

    private:
        void create_instance(ContextOptions const& options);

        void create_debug_messenger();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
                VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                VkDebugUtilsMessageTypeFlagsEXT message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                void* p_user);

        void create_surface(ContextOptions const& options);

        void select_device(ContextOptions const& options);

        void create_device(ContextOptions const& options);

    private:
        vk::Instance m_instance { nullptr };
        VkDebugUtilsMessengerEXT m_debug_messenger { nullptr };
        VkSurfaceKHR m_surface { nullptr };

        vk::PhysicalDevice m_physical_device { nullptr };
        vk::Device m_device { nullptr };

        Queue m_graphics_queue, m_compute_queue, m_present_queue;

        std::vector<std::string> m_enabled_layers;
        std::vector<std::string> m_enabled_instance_extensions;
        std::vector<std::string> m_enabled_device_extensions;
        DeviceFeatures           m_device_features;
    };
}
