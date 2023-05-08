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

        void allocate_memory(vk::MemoryRequirements const& memory_requirements,
                             vk::MemoryPropertyFlags memory_property_flags,
                             vk::DeviceMemory* memory) const;

        const vk::Device& device() const { return m_device; }
        const vk::PhysicalDevice& physical_device() const { return m_physical_device; }

        const VkSurfaceKHR& surface() const { return m_surface; }

        const vk::Instance& instance() const { return m_instance; }

        const Queue& q_compute()  const { return m_compute_queue;  }
        const Queue& q_graphics() const { return m_graphics_queue; }
        const Queue& q_present()  const { return m_present_queue;  }

        const vk::PhysicalDeviceProperties& device_properties() const { return m_physical_device_properties; }

        const DeviceFeatures& device_features() const { return m_device_features; }

        bool is_debug() const { return m_debug_messenger; }

        bool raytracing() const {
            return std::find(
                std::begin(m_enabled_device_extensions),std::end(m_enabled_device_extensions),VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                != std::end(m_enabled_device_extensions);
        }

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

        uint32_t find_memory_type_index(uint32_t filter, vk::MemoryPropertyFlags flags) const;

    private:
        vk::Instance m_instance { nullptr };
        VkDebugUtilsMessengerEXT m_debug_messenger { nullptr };
        VkSurfaceKHR m_surface { nullptr };

        vk::PhysicalDevice m_physical_device { nullptr };
        vk::Device m_device { nullptr };

        Queue m_graphics_queue, m_compute_queue, m_present_queue;

        std::vector<std::string>     m_enabled_layers;
        std::vector<std::string>     m_enabled_instance_extensions;
        std::vector<std::string>     m_enabled_device_extensions;
        vk::PhysicalDeviceProperties m_physical_device_properties;
        DeviceFeatures               m_device_features;
    };
}
