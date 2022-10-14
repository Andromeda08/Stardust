#pragma once

#include <memory>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include "Macro.hpp"
#include "Struct/ApplicationSettings.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Buffer/IndexBuffer.hpp"
#include "Vulkan/Buffer/VertexBuffer.hpp"
#include "Vulkan/Command/CommandBuffers.hpp"
#include "Vulkan/Synchronization/Fence.hpp"
#include "Vulkan/Synchronization/Semaphore.hpp"

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

    void draw();

    std::vector<const char*> deviceExtensions() const { return mRequiredDeviceExtensions; }

    VmaAllocator getAllocator() const { return mAllocator; }

private:
    void setup_vk_debug_msgr()
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info{};

        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = vk_debug_callback;
        // (optional)
        create_info.pUserData = nullptr;

        // Create Debug Messenger
        if (create_vk_debug_msgr_ext(static_cast<VkInstance>(mInstance->handle()), &create_info, nullptr, &mDebugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger.");
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTtype,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    /**
     * @brief Selects a Physical Device that supports VK_KHR_ray_tracing_pipeline and has a Graphics Queue
     * then attempts to create a Logical Device.
     */
    void selectRayTracingDevice();

    /**
     * @brief Builds the swap chain and synchronization objects
     */
    void createSwapChain();

    /**
     * @brief Initialize VMA object
     */
    void createVMA();

    /**
     * @brief Creates the raster Graphics Pipeline
     */
    void createGraphicsPipeline(const std::string& vert_shader_source,
                                const std::string& frag_shader_source);

    /**
     * @brief Free resources.
     */
    void cleanup();

    #pragma region print_utilities

    /**
     * @brief Prints the list of detected physical devices.
     */
    void printDevices();

    #pragma endregion

private:
    ApplicationSettings mSettings {};

    std::unique_ptr<class Window>    mWindow;
    std::unique_ptr<class Instance>  mInstance;
    std::unique_ptr<class Surface>   mSurface;
    std::unique_ptr<class Device>    mDevice;
    std::unique_ptr<Swapchain>       mSwapChain;

    std::unique_ptr<CommandBuffers>  mCommandBuffers;

#pragma region render_test
    std::unique_ptr<class RenderPass> mRenderPass;

    std::unique_ptr<VertexBuffer> mVertexBuffer;
    std::unique_ptr<IndexBuffer> mIndexBuffer;

    vk::PipelineLayout mPipelineLayout;
    vk::Pipeline mGraphicsPipeline;
#pragma endregion

    // Vulkan Memory Allocator
    VmaAllocator mAllocator;

    // Vulkan synchronization objects
    std::vector<Fence>     mInFlightFences = {};
    std::vector<Semaphore> mRenderFinishedSemaphores = {};
    std::vector<Semaphore> mImageAvailableSemaphores = {};

    uint32_t mCurrentFrame {};

    // Device extensions for ray tracing
    const std::vector<const char*> mRequiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };

    VkDebugUtilsMessengerEXT mDebugMessenger;

    static VkResult create_vk_debug_msgr_ext(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );

    static void destroy_vk_debug_msgr_ext(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
    );
};
