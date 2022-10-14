#include "Application.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include "Window.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp"
#include "Vulkan/GraphicsPipeline/RenderPass.hpp"
#include "Vulkan/GraphicsPipeline/ShaderModule.hpp"
#include "Resources/Vertex.hpp"

Application::Application(const ApplicationSettings& app_settings)
: mSettings(app_settings)
, mCurrentFrame(0)
{
    mWindow   = std::make_unique<Window>(app_settings.windowSettings);
    mInstance = std::make_unique<Instance>(*mWindow);

    setup_vk_debug_msgr();

    mSurface  = std::make_unique<Surface>(*mInstance);

    selectRayTracingDevice();

    createSwapChain();

    createVMA();

    mCommandBuffers = std::make_unique<CommandBuffers>(*mDevice);

    mRenderPass = std::make_unique<RenderPass>(*mDevice, mSwapChain->format());

    mSwapChain->createFrameBuffers(*mRenderPass);

    createGraphicsPipeline("basic.vert.spv", "basic.frag.spv");

    mVertexBuffer = std::make_unique<VertexBuffer>(test_vertices, *mCommandBuffers, *mDevice);
    mIndexBuffer = std::make_unique<IndexBuffer>(test_indices, *mCommandBuffers, *mDevice);
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow->handle()))
    {
        glfwPollEvents();

        draw();

        mDevice->waitIdle();
    }

    Application::cleanup();
}

void Application::draw()
{
    auto hDevice = mDevice->handle();

    auto wait = mInFlightFences[mCurrentFrame].handle();
    auto res =hDevice.waitForFences(1, &wait, true, UINT64_MAX);
    res = hDevice.resetFences(1, &wait);

    vk::Semaphore semaphore = mImageAvailableSemaphores[mCurrentFrame].handle();
    auto result = hDevice.acquireNextImageKHR(mSwapChain->handle(), UINT64_MAX, semaphore, nullptr);

    res = hDevice.resetFences(1, &wait);
    auto cmd_buffer = mCommandBuffers->get_buffer(mCurrentFrame);
    cmd_buffer.reset();

#pragma region render_pass_command
    vk::CommandBufferBeginInfo begin_info;
    res = cmd_buffer.begin(&begin_info);
    vk::Rect2D render_area;
    render_area.setExtent(mSwapChain->extent());
    render_area.setOffset({0,0});
    vk::ClearValue clear_color = { std::array<float, 4>{ 0.13f, 0.13f, 0.17f, 1.0f } };
    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.setRenderPass(mRenderPass->handle());
    render_pass_info.setFramebuffer(mSwapChain->framebuffer(mCurrentFrame));
    render_pass_info.setRenderArea(render_area);
    render_pass_info.setClearValueCount(1);
    render_pass_info.setPClearValues(&clear_color);

    cmd_buffer.beginRenderPass(&render_pass_info, vk::SubpassContents::eInline);
    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);

    vk::Viewport viewport;
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth((float) mSwapChain->extent().width);
    viewport.setHeight((float) mSwapChain->extent().height);
    viewport.setMaxDepth(1.0f);
    viewport.setMinDepth(0.0f);
    cmd_buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor;
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(mSwapChain->extent());
    cmd_buffer.setScissor(0, 1, &scissor);

    std::vector<vk::Buffer> vertex_buffers = { mVertexBuffer->handle().handle() };
    std::vector<vk::DeviceSize> offsets = { 0 };
    cmd_buffer.bindVertexBuffers(0, 1, vertex_buffers.data(), offsets.data());
    cmd_buffer.bindIndexBuffer(mIndexBuffer->handle().handle(), 0, vk::IndexType::eUint32);

    cmd_buffer.drawIndexed(static_cast<uint32_t>(test_indices.size()), 1, 0, 0,0);
    cmd_buffer.endRenderPass();
    cmd_buffer.end();
#pragma endregion

    vk::Semaphore wait_semaphores[] = { mImageAvailableSemaphores[mCurrentFrame].handle() };
    vk::Semaphore signal_semaphores[] = { mRenderFinishedSemaphores[mCurrentFrame].handle() };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::SubmitInfo submit_info;
    submit_info.setWaitSemaphoreCount(1);
    submit_info.setPWaitSemaphores(wait_semaphores);
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setCommandBufferCount(1);
    submit_info.setCommandBuffers(cmd_buffer);
    submit_info.setSignalSemaphoreCount(1);
    submit_info.setPSignalSemaphores(signal_semaphores);

    res = mDevice->graphics_queue().submit(1, &submit_info, wait);

    vk::SwapchainKHR swapchains[] = { mSwapChain->handle() };
    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphoreCount(1);
    present_info.setPWaitSemaphores(signal_semaphores);
    present_info.setSwapchainCount(1);
    present_info.setPSwapchains(swapchains);
    present_info.setImageIndices(result.value);
    present_info.setPResults(nullptr);

    res = mDevice->present_queue().presentKHR(&present_info);

    mCurrentFrame = (mCurrentFrame + 1) % 2;
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

    const auto extensions = mRequiredDeviceExtensions;
    mDevice = std::make_unique<Device>(*mSurface, *result, extensions);
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

void Application::createVMA()
{
    VmaAllocatorCreateInfo create_info = {};

    create_info.physicalDevice = mDevice->physicalDevice();
    create_info.device = mDevice->handle();
    create_info.instance = mInstance->handle();

    vmaCreateAllocator(&create_info, &mAllocator);
}

void Application::createGraphicsPipeline(const std::string& vert_shader_source,
                                         const std::string& frag_shader_source)
{
    auto vert_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eVertex,
                                                        vert_shader_source,
                                                        *mDevice);

    auto frag_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eFragment,
                                                        frag_shader_source,
                                                        *mDevice);

    vk::Viewport viewport;
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth((float) mSwapChain->extent().width);
    viewport.setHeight((float) mSwapChain->extent().height);
    viewport.setMaxDepth(1.0f);
    viewport.setMinDepth(0.0f);

    vk::Rect2D scissor;
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(mSwapChain->extent());

    vk::PipelineLayoutCreateInfo create_info;
    create_info.setSetLayoutCount(0);
    create_info.setPSetLayouts(nullptr);

    auto result = mDevice->handle().createPipelineLayout(&create_info, nullptr, &mPipelineLayout);

    GraphicsPipelineState pipeline_state;
    pipeline_state.add_binding_description(Vertex::binding_description());
    pipeline_state.add_attribute_descriptions({
        { 0, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, position)) },
        { 1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, color)) },
        { 2, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, uv)) },
    });
    pipeline_state.add_scissor(scissor);
    pipeline_state.add_viewport(viewport);

    GraphicsPipelineBuilder builder(*mDevice, mPipelineLayout, *mRenderPass, pipeline_state);
    builder.add_shader(*vert_shader);
    builder.add_shader(*frag_shader);

    mGraphicsPipeline = builder.create_pipeline();
}

void Application::cleanup()
{
    mSwapChain->destroy();
}

#pragma region printer_utilities

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

VKAPI_ATTR VkBool32 VKAPI_CALL Application::vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTtype,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

VkResult Application::create_vk_debug_msgr_ext(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    // If (func == nullptr) the function wasn't loaded.
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Application::destroy_vk_debug_msgr_ext(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
