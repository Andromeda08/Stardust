#include "Context.hpp"

#include <iostream>
#include <ranges>
#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace sdvk
{
    Context::Context(const ContextOptions& options)
    {
        vk::DynamicLoader dl;
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        create_instance(options);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

        if (options.debug)
        {
            create_debug_messenger();
        }

        if (options.with_surface)
        {
            create_surface(options);
        }

        select_device(options);

        create_device(options);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
    }

    void Context::create_instance(const ContextOptions& options)
    {
        auto available_layers = vk::enumerateInstanceLayerProperties();
        auto available_extensions = vk::enumerateInstanceExtensionProperties();

        std::set<std::string> required_layers, required_extensions;
        for (const auto& l : options.instance_layers)
        {
            required_layers.insert(l);
        }
        for (const auto& e : options.instance_extensions)
        {
            required_extensions.insert(e);
        }

        if (options.validation)
        {
            required_layers.insert("VK_LAYER_KHRONOS_validation");
        }

        if (options.debug)
        {
            required_extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            required_extensions.insert("VK_EXT_debug_utils");
        }

        std::vector<const char*> found_layers;
        for (const auto& layer : available_layers)
        {
            if (required_layers.count(layer.layerName.data()))
            {
                found_layers.push_back(layer.layerName);
            }
        }

        std::vector<const char*> found_extensions;
        for (const auto& extension : available_extensions)
        {
            if (required_extensions.count(extension.extensionName.data()))
            {
                found_extensions.push_back(extension.extensionName);
            }
        }

        vk::ApplicationInfo application_info;
        application_info.setApiVersion(VK_API_VERSION_1_3);

        vk::InstanceCreateInfo create_info;
        create_info.setPApplicationInfo(&application_info);
        create_info.setEnabledLayerCount(found_layers.size());
        create_info.setPpEnabledLayerNames(found_layers.data());
        create_info.setEnabledExtensionCount(found_extensions.size());
        create_info.setPpEnabledExtensionNames(found_extensions.data());

        vk::Result result = vk::createInstance(&create_info, nullptr, &m_instance);

        std::copy(found_layers.begin(), found_layers.end(), std::back_inserter(m_enabled_layers));
        std::copy(found_extensions.begin(), found_extensions.end(), std::back_inserter(m_enabled_instance_extensions));
    }

    void Context::create_debug_messenger()
    {
        auto severities = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

        auto types = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

        vk::DebugUtilsMessengerCreateInfoEXT create_info;
        create_info.setMessageSeverity(severities);
        create_info.setMessageType(types);
        create_info.setPfnUserCallback((PFN_vkDebugUtilsMessengerCallbackEXT)(Context::debug_callback));

        bool error = false;
        auto fun = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(static_cast<VkInstance>(m_instance), "vkCreateDebugUtilsMessengerEXT");

        if (fun)
        {
            auto result = fun(m_instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&create_info), nullptr, &m_debug_messenger);
            error = (result != VK_SUCCESS);
        }
        else
        {
            error = true;
        }

        if (error)
        {
            throw std::runtime_error("Failed to setup VkDebugUtilsMessengerEXT.");
        }
    }

    VkBool32 Context::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                     const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                     void* p_user_data)
    {
        if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cerr << p_callback_data->pMessage << std::endl;
        }

        return VK_FALSE;
    }

    void Context::create_surface(const ContextOptions& options)
    {
        VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(m_instance), options.window, nullptr, &m_surface);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface.");
        }
    }

    void Context::select_device(const ContextOptions& options)
    {
        std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();
        physical_devices | std::views::filter([](vk::PhysicalDevice const& pd){
            auto props = pd.getProperties();
            return props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
                || props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu;
        });

        const auto candidate = std::find_if(
                std::begin(physical_devices), std::end(physical_devices),
                [options](vk::PhysicalDevice const& device) {
                    const auto supported_extensions = device.enumerateDeviceExtensionProperties();
                    const auto queue_families = device.getQueueFamilyProperties();
                    const auto props = device.getProperties();

                    bool has_ray_tracing = std::find_if(
                            std::begin(supported_extensions), std::end(supported_extensions),
                            [](vk::ExtensionProperties const& extension_properties) {
                                return std::string(extension_properties.extensionName.data()) == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
                            }) != std::end(supported_extensions);

                    bool has_graphics_queue = std::find_if(
                            std::begin(queue_families), std::end(queue_families),
                            [](vk::QueueFamilyProperties const& properties) {
                                return properties.queueFlags & vk::QueueFlagBits::eGraphics;
                            }) != std::end(queue_families);

                    bool is_discrete = props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;

                    std::set<std::string> required_extensions(std::begin(options.device_extensions), std::end(options.device_extensions));
                    for (const auto& extension : supported_extensions) {
                        required_extensions.erase(extension.extensionName);
                    }

                    bool has_missing_extensions = !required_extensions.empty();

                    bool result = has_graphics_queue && !has_missing_extensions;

                    if (options.raytracing)
                    {
                        // Some Radeon iGPUs are RT capable, pick only discrete for now since we don't score GPUs.
                        return result && has_ray_tracing && is_discrete;
                    }

                    return result;
                });

        if (candidate == std::end(physical_devices))
        {
            throw std::runtime_error("Could not find a suitable GPU.");
        }

        m_physical_device = *candidate;

        if (options.debug)
        {
            std::cout << "Detected devices:" << std::endl;
            for (const auto& pd : physical_devices) {
                auto props = pd.getProperties();
                std::cout << "- " << props.deviceName << " [" << to_string(props.deviceType) << "]" << std::endl;
            }
            m_physical_device_properties = m_physical_device.getProperties();
            std::cout << "Selected device: " << m_physical_device_properties.deviceName << std::endl;
        }
    }

    void Context::create_device(const ContextOptions& options)
    {
        std::vector<const char*> validation_layers, extensions;
        for (const auto& s : m_enabled_layers)
        {
            validation_layers.push_back(s.c_str());
        }
        for (const auto& s : options.device_extensions)
        {
            extensions.push_back(s);
            m_enabled_device_extensions.emplace_back(s);
        }

        const auto queue_families = m_physical_device.getQueueFamilyProperties();
        auto get_queue_index = [queue_families](vk::QueueFlagBits req, vk::QueueFlagBits exc){
            const auto family = std::find_if(
                    std::begin(queue_families), std::end(queue_families),
                    [req, exc](const vk::QueueFamilyProperties& props) {
                        return (props.queueCount > 0 && (props.queueFlags & req) && !(props.queueFlags & exc));
                    });

            if (family == queue_families.end())
            {
                std::stringstream msg;
                msg << "Failed to find queue family with required flag: \"";
                msg << string_VkQueueFlagBits(static_cast<VkQueueFlagBits>(req)) << "\".";
                throw std::runtime_error(msg.str());
            }

            return static_cast<uint32_t>(family - std::begin(queue_families));
        };
        auto get_present_index = [&, queue_families](){
            uint32_t i = 0, result = 0;
            for (const auto& props : queue_families)
            {
                if (m_physical_device.getSurfaceSupportKHR(i, m_surface))
                {
                    result = i;
                }
                i++;
            }
            return result;
        };

        auto graphics = get_queue_index(vk::QueueFlagBits::eGraphics, {});
        auto compute = get_queue_index(vk::QueueFlagBits::eCompute, vk::QueueFlagBits::eGraphics);
        auto present = get_present_index();

        float queue_priority = 1.0f;
        std::set<uint32_t> unique_queue_indices = { graphics, compute, present };
        std::vector<vk::DeviceQueueCreateInfo> queue_infos;
        for (auto i : unique_queue_indices)
        {
            vk::DeviceQueueCreateInfo queue_info;
            queue_info.setQueueFamilyIndex(i);
            queue_info.setQueueCount(1);
            queue_info.setPQueuePriorities(&queue_priority);
            queue_infos.push_back(queue_info);
        }

        m_device_features.device_features.setSamplerAnisotropy(true);
        m_device_features.device_features.setShaderInt64(true);
        m_device_features.device_features.setFillModeNonSolid(true);

        vk::DeviceCreateInfo create_info;
        create_info.setEnabledLayerCount(validation_layers.size());
        create_info.setPpEnabledLayerNames(validation_layers.data());
        create_info.setEnabledExtensionCount(extensions.size());
        create_info.setPpEnabledExtensionNames(extensions.data());
        create_info.setPEnabledFeatures(&m_device_features.device_features);
        create_info.setQueueCreateInfoCount(queue_infos.size());
        create_info.setPQueueCreateInfos(queue_infos.data());

        if (options.raytracing)
        {
            m_device_features.with_ray_tracing();
            create_info.setPNext(&m_device_features.ray_tracing_pipeline);
        }

        vk::Result result = m_physical_device.createDevice(&create_info, nullptr, &m_device);

        m_graphics_queue.index = graphics;
        m_device.getQueue(graphics, 0, &m_graphics_queue.queue);
        m_compute_queue.index = compute;
        m_device.getQueue(compute, 0, &m_compute_queue.queue);
        m_present_queue.index = present;
        m_device.getQueue(present, 0, &m_present_queue.queue);

        if (options.debug)
        {
            vk::DebugUtilsObjectNameInfoEXT name_info;
            name_info.setObjectHandle((uint64_t) static_cast<VkQueue>(m_graphics_queue.queue));
            name_info.setObjectType(vk::ObjectType::eQueue);
            name_info.setPObjectName("Graphics Queue");
            result = m_device.setDebugUtilsObjectNameEXT(&name_info);

            name_info.setObjectHandle((uint64_t) static_cast<VkQueue>(m_compute_queue.queue));
            name_info.setObjectType(vk::ObjectType::eQueue);
            name_info.setPObjectName("Compute Queue");
            result = m_device.setDebugUtilsObjectNameEXT(&name_info);

            name_info.setObjectHandle((uint64_t) static_cast<VkQueue>(m_present_queue.queue));
            name_info.setObjectType(vk::ObjectType::eQueue);
            name_info.setPObjectName("Present Queue");
            result = m_device.setDebugUtilsObjectNameEXT(&name_info);
        }
    }

    void Context::allocate_memory(vk::MemoryRequirements const& memory_requirements,
                                  vk::MemoryPropertyFlags memory_property_flags,
                                  vk::DeviceMemory* memory) const
    {
        auto type_index = find_memory_type_index(memory_requirements.memoryTypeBits, memory_property_flags);
        vk::MemoryAllocateInfo alloc_info { memory_requirements.size, type_index };
        auto result = m_device.allocateMemory(&alloc_info, nullptr, memory);
    }

    uint32_t Context::find_memory_type_index(uint32_t filter, vk::MemoryPropertyFlags flags) const
    {
        auto props = m_physical_device.getMemoryProperties();
        for (uint32_t i = 0; i < props.memoryTypeCount; i++)
        {
            if ((filter & (1 << i)) && (props.memoryTypes[i].propertyFlags & flags) == flags)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }
}