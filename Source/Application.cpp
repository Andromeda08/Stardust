#include "Application.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <Window.hpp>
#include <Scenes/Scene.hpp>
#include <Scenes/Raytracing.hpp>
#include <Scenes/Voxels.hpp>
#include <vk/Device/Device.hpp>
#include <vk/Device/Instance.hpp>
#include <vk/Device/Surface.hpp>

void keyboard_input_handler(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

Application::Application(const ApplicationSettings& app_settings)
: mSettings(app_settings)
{
#if defined(__APPLE__)
    // Yeah, we're not going to do this any time soon.
    mSettings.raytracing = false;
#endif

    mWindow = std::make_unique<Window>(app_settings.windowSettings);

    glfwSetKeyCallback(mWindow->handle(), keyboard_input_handler);

    mInstance = std::make_unique<Instance>(*mWindow);

    setupDebugMessenger();

    mSurface = std::make_unique<Surface>(*mInstance);

    if (mSettings.raytracing)
    {
        try { selectRayTracingDevice(); }
        catch (const std::runtime_error& e) {
            std::cout << "Failed to find device with ray tracing support.\nRetrying without rt support...";
            mSettings.raytracing = false;
            selectDevice();
        }
    }
    else
    {
        selectDevice();
    }

    createSwapChain();

    mCommandBuffers = std::make_unique<CommandBuffers>(*mDevice);

    mSceneManager = std::make_unique<sd::SceneManager>();

    // Scene "generators"
    const auto gen_raster_scene = [&]() {
        auto scene = std::make_unique<sd::Scene>(*mSwapChain, *mCommandBuffers);
        scene->add_object("sphere", "default", Math::model(glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), "SphereR");
        scene->add_object("sphere", "default", Math::model(glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), "SphereG");
        scene->add_object("sphere", "default", Math::model(glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), "SphereB");
        scene->add_object("sphere", "default", Math::model(glm::vec3(0.0f, 0.0f, 0.0f)), glm::vec4(1.0f), "SphereW");
        return scene;
    };
    const auto gen_raytracing_scene = [&]() {
        return std::make_unique<sd::Raytracing>(*mSwapChain, *mCommandBuffers);
    };
    const auto gen_instanced_scene = [&]() {
        return std::make_unique<sd::Voxels>(glm::ivec2{ 1024, 1024 }, *mSwapChain, *mCommandBuffers);
    };

    mSceneManager->add_scene(gen_raster_scene, "Raster");
    mSceneManager->add_scene(gen_raytracing_scene, "Raytracing");
    mSceneManager->add_scene(gen_instanced_scene, "Instanced");

    name_vk_objects();
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow->handle()))
    {
        glfwPollEvents();

        mSceneManager->register_keybinds(mWindow->handle());
        render();

        mDevice->waitIdle();

        //glfwSetWindowShouldClose(mWindow->handle(), true);
    }

    Application::cleanup();
}

void Application::render()
{
    auto acquired_index = begin_frame();
    vk::Result result;

    auto cmd = mCommandBuffers->operator[](mCurrentFrame);
    vk::CommandBufferBeginInfo begin_info;
    result = cmd.begin(&begin_info);
    {
        mSceneManager->render_active_scene(mCurrentFrame, cmd);
    }
    cmd.end();

    submit_frame(cmd, acquired_index);

    mCurrentFrame = (mCurrentFrame + 1) % 2;
}

uint32_t Application::begin_frame()
{
    vk::Result result;
    auto fence = mInFlightFences[mCurrentFrame].handle();
    result = mDevice->handle().waitForFences(1, &fence, true, std::numeric_limits<uint64_t>::max());
    result = mDevice->handle().resetFences(1, &fence);

    return mDevice->handle().acquireNextImageKHR(mSwapChain->handle(),
                                                 std::numeric_limits<uint64_t>::max(),
                                                 mImageAvailableSemaphores[mCurrentFrame].handle(),
                                                 nullptr).value;
}

void Application::submit_frame(const vk::CommandBuffer& command_buffer, uint32_t acquired_index)
{
    vk::Result result;

    vk::Semaphore wait_semaphores[] = { mImageAvailableSemaphores[mCurrentFrame].handle() };
    vk::Semaphore signal_semaphores[] = { mRenderFinishedSemaphores[mCurrentFrame].handle() };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

    vk::SubmitInfo submit_info;
    submit_info.setWaitSemaphoreCount(1);
    submit_info.setPWaitSemaphores(wait_semaphores);
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setCommandBufferCount(1);
    submit_info.setCommandBuffers(command_buffer);
    submit_info.setSignalSemaphoreCount(1);
    submit_info.setPSignalSemaphores(signal_semaphores);
    result = mDevice->graphics_queue().submit(1, &submit_info, mInFlightFences[mCurrentFrame].handle());

    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphoreCount(1);
    present_info.setPWaitSemaphores(signal_semaphores);
    present_info.setSwapchainCount(1);
    present_info.setPSwapchains(&mSwapChain->handle());
    present_info.setImageIndices(acquired_index);
    present_info.setPResults(nullptr);
    result = mDevice->present_queue().presentKHR(&present_info);
}

void Application::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT create_info { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

    create_info.messageSeverity = Debug::vk_debug_msg_severities();
    create_info.messageType     = Debug::vk_debug_msg_types();
    create_info.pfnUserCallback = Debug::vk_debug_callback;

    auto result = Debug::create_vk_debug_msgr_ext(static_cast<VkInstance>(mInstance->handle()),
                                                  &create_info,
                                                  nullptr,
                                                  &mDebugMessenger);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger.");
    }
}

void Application::selectRayTracingDevice()
{
    const auto& physicalDevices = mInstance->physicalDevices();

    const auto result = std::find_if(
        std::begin(physicalDevices), std::end(physicalDevices),
        [](const vk::PhysicalDevice& device) {
            // 1. Query ray tracing support
            const auto extensions = device.enumerateDeviceExtensionProperties();
            const auto hasRayTracing = std::find_if(
                std::begin(extensions), std::end(extensions),
                [](const vk::ExtensionProperties& extensionProperties) {
                    return std::string(extensionProperties.extensionName.data()) == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
                });

            // 2. Query if the device has a graphics queue or not
            const auto queueFamilies = device.getQueueFamilyProperties();
            const auto hasGraphicsQueue = std::find_if(
                std::begin(queueFamilies), std::end(queueFamilies),
                [](const vk::QueueFamilyProperties& queueFamilyProperties) {
                    return queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics;
                });

            return (hasRayTracing != std::end(extensions))
                && (hasGraphicsQueue != std::end(queueFamilies));
        });

    if (result == std::end(physicalDevices))
    {
        throw std::runtime_error("Couldn't find a ray tracing capable device.");
    }

    if (mSettings.logging)
    {
        Application::printDevices();
        auto props = result->getProperties();
        std::cout << "Selected device: " << props.deviceName << std::endl;
    }

    auto extensions = mRequiredDeviceExtensions;
    extensions.insert(std::end(extensions), std::begin(mRaytracingDeviceExtensions), std::end(mRaytracingDeviceExtensions));
    mDevice = std::make_unique<Device>(*mInstance, *mSurface, *result, extensions);
}

void Application::selectDevice()
{
    const auto& devices = mInstance->physicalDevices();
    auto result = devices[0];

    if (mSettings.logging)
    {
        Application::printDevices();
        auto props = result.getProperties();
        std::cout << "Selected device: " << props.deviceName << std::endl;
    }

    mDevice = std::make_unique<Device>(*mInstance, *mSurface, result, mRequiredDeviceExtensions);
}

void Application::createSwapChain()
{
    mSwapChain = std::make_unique<Swapchain>(*mDevice);

    for (uint32_t i = 0; i < 2; i++)
    {
        mImageAvailableSemaphores.emplace_back(*mDevice);
        mRenderFinishedSemaphores.emplace_back(*mDevice);
        mInFlightFences.emplace_back(*mDevice);
    }
}

void Application::cleanup()
{
    mSwapChain->cleanup();

    Debug::destroy_vk_debug_msgr_ext(static_cast<VkInstance>(mInstance->handle()),
                                     mDebugMessenger,
                                     nullptr);
}

#pragma region debug_and_print_utilities

void Application::name_vk_objects()
{
    // Name swapchain images.
    for (size_t i = 0; i < mSwapChain->image_count(); i++)
    {
        std::stringstream ss {};
        ss << "Swapchain image [" << std::to_string(i) << "]";

        vk::DebugUtilsObjectNameInfoEXT swapchain_image;
        swapchain_image.setObjectHandle((uint64_t) static_cast<VkImage>(mSwapChain->operator[](i)));
        swapchain_image.setObjectType(vk::ObjectType::eImage);
        swapchain_image.setPObjectName(ss.str().c_str());
        auto result = mDevice->handle().setDebugUtilsObjectNameEXT(&swapchain_image, mDevice->dispatch());
    }
}

void Application::printDevices()
{
    const auto& foundDevices = mInstance->physicalDevices();
    std::cout << "Detected devices:" << std::endl;
    for (const auto& pd : foundDevices) {
        auto props = pd.getProperties();
        std::cout << "- " << props.deviceName << " [" << to_string(props.deviceType) << "]" << std::endl;
    }
}

#pragma endregion
