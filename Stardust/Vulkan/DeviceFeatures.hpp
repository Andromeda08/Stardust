#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    struct DeviceFeatures
    {
        vk::PhysicalDeviceFeatures device_features;
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure;
        vk::PhysicalDeviceBufferDeviceAddressFeaturesEXT buffer_device_address;
        vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing;
        vk::PhysicalDeviceMaintenance4FeaturesKHR maintenance4;
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline;
        vk::PhysicalDeviceSynchronization2FeaturesKHR synchronization2;
        vk::PhysicalDeviceRayQueryFeaturesKHR ray_query;
        vk::PhysicalDeviceTimelineSemaphoreFeatures timeline_semaphores;
        vk::PhysicalDeviceMeshShaderFeaturesEXT mesh_shader;

        DeviceFeatures() = default;

        void with_ray_tracing()
        {
            ray_query.setRayQuery(true);
            synchronization2.setSynchronization2(true);
            synchronization2.setPNext(&ray_query);
            descriptor_indexing.setRuntimeDescriptorArray(true);
            descriptor_indexing.setPNext(&synchronization2);
            acceleration_structure.setAccelerationStructure(true);
            acceleration_structure.setPNext(&descriptor_indexing);
            ray_tracing_pipeline.setRayTracingPipeline(true);
            ray_tracing_pipeline.setPNext(&acceleration_structure);
        }
    };
}

namespace Nebula::Vulkan
{
    enum class DeviceVendor
    {
        eAMD = 0x1002,
        eNVIDIA = 0x10de,
        eIntel = 0x8086,
        eArm = 0x13b5,
    };

    struct DeviceFeatures
    {
        // Vulkan 1.1
        vk::PhysicalDeviceFeatures enabled_features = {};
        vk::PhysicalDeviceFeatures2 enabled_features_2 = {};
        vk::PhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features = {};
        vk::PhysicalDeviceSamplerYcbcrConversionFeatures sampler_ycbcr_conversion_features = {};
        vk::PhysicalDeviceIDProperties id_properties = {};

        // Vulkan 1.2
        vk::PhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {};
        vk::PhysicalDeviceSynchronization2Features synchronization_2_features = {};
        vk::PhysicalDeviceShaderFloat16Int8Features shader_float_16_int_8_features = {};

        // KHR
        vk::PhysicalDeviceMaintenance4FeaturesKHR maintenance4_features = {};
        vk::PhysicalDeviceRayQueryFeaturesKHR ray_query_features;
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features = {};
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features = {};

        // EXT
        vk::PhysicalDeviceBufferDeviceAddressFeaturesEXT buffer_device_address = {};
        vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptor_indexing = {};

        std::vector<const char*> device_extensions;
    };
}