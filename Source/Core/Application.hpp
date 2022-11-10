#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>
#include "Macro.hpp"
#include "Resources/Geometry.hpp"
#include "Struct/ApplicationSettings.hpp"
#include "Vulkan/DebugMessenger.hpp"
#include "Vulkan/DepthBuffer.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Buffer/IndexBuffer.hpp"
#include "Vulkan/Buffer/InstanceBuffer.hpp"
#include "Vulkan/Buffer/UniformBuffer.hpp"
#include "Vulkan/Buffer/VertexBuffer.hpp"
#include "Vulkan/Command/CommandBuffers.hpp"
#include "Vulkan/Descriptor/DescriptorSets.hpp"
#include "Vulkan/Synchronization/Fence.hpp"
#include "Vulkan/Synchronization/Semaphore.hpp"
#include "Scene/Mesh.hpp"

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

    std::vector<const char*> deviceExtensions() const { return mRequiredDeviceExtensions; }

private:
    /**
     * @brief Draw frame :D
     */
    void draw();

    /**
     * @brief Sets up the vulkan debug messenger
     */
    void setupDebugMessenger();

    /**
     * @brief Selects a Physical Device that supports VK_KHR_ray_tracing_pipeline and has a Graphics Queue
     * then attempts to create a Logical Device.
     */
    void selectRayTracingDevice();

    void selectDevice();

    /**
     * @brief Builds the swap chain and synchronization objects
     */
    void createSwapChain();

    /**
     * @brief Creates the raster Graphics Pipeline
     */
    void createGraphicsPipeline(const std::string& vert_shader_source, const std::string& frag_shader_source);

    /**
     * @brief Updates the specified uniform buffer.
     */
    void updateUniformBuffer(size_t index);

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

    VkDebugUtilsMessengerEXT mDebugMessenger;

    std::unique_ptr<class Window>    mWindow;
    std::unique_ptr<class Instance>  mInstance;
    std::unique_ptr<class Surface>   mSurface;
    std::unique_ptr<class Device>    mDevice;
    std::unique_ptr<Swapchain>       mSwapChain;
    std::unique_ptr<DepthBuffer>     mDepthBuffer;
    std::unique_ptr<CommandBuffers>  mCommandBuffers;

#pragma region render_test
    std::unique_ptr<class RenderPass> mRenderPass;

    std::unique_ptr<Mesh> mTestMesh;

    std::vector<IData> mInstanceData;
    std::unique_ptr<Geometry> mGeometry;

    std::unique_ptr<VertexBuffer>               mVertexBuffer;
    std::unique_ptr<IndexBuffer>                mIndexBuffer;
    std::unique_ptr<InstanceBuffer>             mInstanceBuffer;
    std::vector<std::unique_ptr<UniformBuffer>> mUniformBuffers;

    vk::DescriptorSetLayout         mDescriptorSetLayout;
    std::unique_ptr<DescriptorSets> mDescriptorSets;

    vk::PipelineLayout mPipelineLayout;
    vk::Pipeline       mGraphicsPipeline;

    uint32_t mCurrentFrame {};
#pragma endregion

    std::vector<Fence>     mInFlightFences = {};
    std::vector<Semaphore> mRenderFinishedSemaphores = {};
    std::vector<Semaphore> mImageAvailableSemaphores = {};

    std::vector<const char*> mDefaultExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    // Device extensions with ray tracing in mind
    const std::vector<const char*> mRequiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
};
