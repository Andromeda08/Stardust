#pragma once

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

        DeviceFeatures() = default;

        void with_ray_tracing()
        {
            ray_query.setRayQuery(true);
            synchronization2.setSynchronization2(true);
            synchronization2.setPNext(&ray_query);
            maintenance4.setMaintenance4(true);
            maintenance4.setPNext(&synchronization2);
            descriptor_indexing.setRuntimeDescriptorArray(true);
            descriptor_indexing.setPNext(&maintenance4);
            buffer_device_address.setBufferDeviceAddress(true);
            buffer_device_address.setPNext(&descriptor_indexing);
            acceleration_structure.setAccelerationStructure(true);
            acceleration_structure.setPNext(&buffer_device_address);
            ray_tracing_pipeline.setRayTracingPipeline(true);
            ray_tracing_pipeline.setPNext(&acceleration_structure);
        }
    };
}