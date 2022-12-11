#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <Struct/ApplicationSettings.hpp>
#include <Utility/Macro.hpp>
#include <Vulkan/DebugMessenger.hpp>
#include <Vulkan/Swapchain.hpp>
#include <Vulkan/Command/CommandBuffer.hpp>
#include <Vulkan/Descriptor/DescriptorSets.hpp>
#include <Vulkan/Synchronization/Fence.hpp>
#include <Vulkan/Synchronization/Semaphore.hpp>
#include <vk/Scene.hpp>
#include <vk/RayTracingScene.hpp>
#include <Scenes/Terrain/TerrainScene.hpp>

class Application {
public:
    NON_COPIABLE(Application)

    /**
     * @brief Create Application with specified settings
     */
    explicit Application(const ApplicationSettings& app_settings);

    /**
     * @brief Application main loop
     */
    void run();

private:
    void rasterize();

    void raytrace();

    /**
     * @brief Sets up the vulkan debug messenger
     */
    void setupDebugMessenger();

    /**
     * @brief Selects a Physical Device that supports VK_KHR_ray_tracing_pipeline and has a Graphics Queue
     * then attempts to create a Logical Device.
     */
    void selectRayTracingDevice();

    /**
     * @brief Selects the first discrete GPU it finds for rendering.
     */
    void selectDevice();

    /**
     * @brief Builds the swap chain and synchronization objects
     */
    void createSwapChain();

    /**
     * @brief Free resources.
     */
    void cleanup();

    /**
     * @brief Names a couple of vulkan objects for debugging.
     */
    void name_vk_objects();

    /**
     * @brief Prints the list of detected physical devices.
     */
    void printDevices();

private:
    ApplicationSettings             mSettings {};
    VkDebugUtilsMessengerEXT        mDebugMessenger;
    std::unique_ptr<class Window>   mWindow;
    std::unique_ptr<class Instance> mInstance;
    std::unique_ptr<class Surface>  mSurface;
    std::unique_ptr<class Device>   mDevice;
    std::unique_ptr<Swapchain>      mSwapChain;
    std::unique_ptr<CommandBuffer>  mCommandBuffers;

#pragma region rendering

    std::unique_ptr<re::Scene>           mScene1;
    std::unique_ptr<re::RayTracingScene> mScene2;
    std::unique_ptr<TerrainScene>        mScene3;
    uint32_t                             mCurrentFrame {0};

#pragma endregion

    std::vector<Fence>     mInFlightFences = {};
    std::vector<Semaphore> mRenderFinishedSemaphores = {};
    std::vector<Semaphore> mImageAvailableSemaphores = {};

#pragma region device_extensions

const std::vector<const char*> mRequiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(__APPLE__)
        "VK_KHR_portability_subset"
#endif
    };
    const std::vector<const char*> mRaytracingDeviceExtensions = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
};

#pragma endregion
