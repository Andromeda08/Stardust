#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include <Scenes/SceneManager.hpp>
#include <Struct/ApplicationSettings.hpp>
#include <vk/Commands/CommandBuffers.hpp>
#include <vk/Device/DebugMessenger.hpp>
#include <vk/Presentation/Swapchain.hpp>
#include <vk/Synchronization/Fence.hpp>
#include <vk/Synchronization/Semaphore.hpp>

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
    /**
     * @breif Render currently active scene.
     */
    void render();

    /**
     * @brief Acquire the next swapchain image for presentation.
     * @returns Acquired swapchain image index.
     */
    uint32_t begin_frame();

    /**
     * @brief Submit and present the current frame.
     * @param command_buffer Rendering command buffer
     * @param acquired_index Swapchain image index
     */
    void submit_frame(const vk::CommandBuffer& command_buffer, uint32_t acquired_index);

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
    ApplicationSettings              mSettings {};
    VkDebugUtilsMessengerEXT         mDebugMessenger {};
    std::unique_ptr<class Window>    mWindow;
    std::unique_ptr<class Instance>  mInstance;
    std::unique_ptr<class Surface>   mSurface;
    std::unique_ptr<class Device>    mDevice;
    std::unique_ptr<Swapchain>       mSwapChain;
    std::unique_ptr<CommandBuffers>  mCommandBuffers;

#pragma region rendering

    std::unique_ptr<sd::SceneManager> mSceneManager;
    uint32_t                          mCurrentFrame {0};

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
