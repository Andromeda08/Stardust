#pragma once

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "ContextOptions.hpp"
#include "DeviceFeatures.hpp"
#include "Queues.hpp"
#include "Utils.hpp"

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

        const vk::SurfaceKHR& surface() const { return m_surface; }

        const vk::Instance& instance() const { return m_instance; }

        const Queue& q_compute()  const { return m_compute_queue;  }

        const Queue& q_graphics() const { return m_graphics_queue; }

        const Queue& q_present()  const { return m_present_queue;  }

        const vk::PhysicalDeviceProperties& device_properties() const { return m_physical_device_properties; }

        const DeviceFeatures& device_features() const { return m_device_features; }

        bool is_debug() const { return m_debug_messenger; }

        bool is_raytracing_capable() const;

    private:
        void create_instance(ContextOptions const& options);

        void create_debug_messenger();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
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
        vk::SurfaceKHR m_surface;

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

namespace Nebula::Vulkan
{
    struct QueueInfo
    {
        struct Queue
        {
            uint32_t  index { 0 };
            vk::Queue queue { nullptr };
        } graphics, compute;
    };

    /**
     * Allows for a customizable GPU selection process.
     */
    struct GpuSelector
    {
        virtual ~GpuSelector() = default;
        virtual vk::PhysicalDevice select_gpu(std::vector<vk::PhysicalDevice> gpus) = 0;
    };

    struct DefaultGpuSelector : public GpuSelector
    {
        explicit DefaultGpuSelector(const std::vector<const char*>& extensions): GpuSelector(), _extensions(extensions) {}

        vk::PhysicalDevice select_gpu(std::vector<vk::PhysicalDevice> gpus) override;

    private:
        const std::vector<const char*>& _extensions;
    };

    struct ContextCreateParams
    {
        std::vector<const char*> instance_extensions;
        std::vector<const char*> device_extensions;
        GpuSelector*             gpu_selector;
        GLFWwindow*              with_window;
        bool                     with_surface;
        bool                     with_debug;
    };

    class Context
    {
    public:
        struct Builder
        {
            Builder& with_gpu_selector(GpuSelector* gpu_selector)
            {
                params.gpu_selector = gpu_selector;
                return *this;
            }

            Builder& with_instance_extensions(const std::vector<const char*>& extensions)
            {
                auto& exts = params.instance_extensions;
                exts.insert(std::end(exts), std::begin(extensions), std::end(extensions));
                return *this;
            }

            Builder& with_device_extensions(const std::vector<const char*>& extensions)
            {
                auto& exts = params.device_extensions;
                exts.insert(std::end(exts), std::begin(extensions), std::end(extensions));
                return *this;
            }

            Builder& with_surface(GLFWwindow* window)
            {
                params.with_window = window;
                params.with_surface = true;
                return *this;
            }

            std::shared_ptr<Context> create_context()
            {
                return std::make_shared<Context>(params);
            }

        private:
            ContextCreateParams params;
        };

        explicit Context(ContextCreateParams  create_params);

        Context(const Context&) = delete;
        void operator=(const Context&) = delete;

        ~Context() = default;

        vk::Instance get_instance() const { return m_instance; }

        vk::PhysicalDevice get_gpu() const { return m_gpu; }

        vk::Device get_device() const { return m_device; }

        vk::SurfaceKHR get_surface() const { return m_surface; }

    private:
        void _create_instance();

        void _select_gpu();

        void _create_device();

        void _initialize_debug_features();

        void _create_surface();

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                             VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                             void* p_user);

    private:
        std::shared_ptr<GpuSelector> m_gpu_selector;

        vk::Device         m_device   { nullptr };
        vk::Instance       m_instance { nullptr };
        vk::PhysicalDevice m_gpu      { nullptr };
        vk::SurfaceKHR     m_surface  { nullptr };

        QueueInfo          m_queue_info {};
        DeviceFeatures     m_device_features {};

        vk::PhysicalDeviceProperties m_gpu_properties {};
        vk::PhysicalDeviceMemoryProperties m_gpu_memory_properties {};

        vk::PhysicalDeviceFeatures2 m_gpu_features_2 {};
        std::vector<const char*> m_enabled_device_extensions;
        std::vector<const char*> m_enabled_instance_extensions;

        VkDebugUtilsMessengerEXT m_debug_messenger;

        ContextCreateParams m_create_params;
    };
}