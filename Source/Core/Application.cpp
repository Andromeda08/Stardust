#include "Application.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

    setupDebugMessenger();

    mSurface  = std::make_unique<Surface>(*mInstance);

    selectRayTracingDevice();

    createSwapChain();

    mCommandBuffers = std::make_unique<CommandBuffers>(*mDevice);

    mDepthBuffer = std::make_unique<DepthBuffer>(mSwapChain->extent(), *mDevice, *mCommandBuffers);

    mRenderPass = std::make_unique<RenderPass>(*mDevice, mSwapChain->format(), mDepthBuffer->format());

    mSwapChain->createFrameBuffers(*mRenderPass, *mDepthBuffer);

    auto ubo_binding = UniformBuffer::layout_binding(0);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = { ubo_binding };

    vk::DescriptorSetLayoutCreateInfo layout_create_info;
    layout_create_info.setBindingCount(bindings.size());
    layout_create_info.setPBindings(bindings.data());
    auto result = mDevice->handle().createDescriptorSetLayout(&layout_create_info, nullptr, &mDescriptorSetLayout);

    createGraphicsPipeline("phong.vert.spv", "phong.frag.spv");

    //mGeometry = std::make_unique<SphereGeometry>(1.5f, glm::vec3{0.5f, 0.5f, 0.5f}, 60);
    mGeometry = std::make_unique<CubeGeometry>(1.5f);

    std::default_random_engine random((unsigned)time(nullptr));
    std::uniform_int_distribution<int> uniform_dist(-600, 600);
    std::uniform_real_distribution<float> uniform_float(0.0f, 1.0f);
    std::uniform_real_distribution<float> scale_mod(1.0f, 5.0f);

    for (int i = 0; i < 8128; i++)
    {
        float x = (float) uniform_dist(random);
        float y = (float) uniform_dist(random);
        float z = (float) uniform_dist(random);

        mInstanceData.push_back({
            glm::vec3{ x, y, z },
            glm::vec3{ uniform_float(random), uniform_float(random), uniform_float(random) },
            glm::vec3{ scale_mod(random) }
        });
    }

    mInstanceBuffer = std::make_unique<InstanceBuffer>(mInstanceData, *mCommandBuffers, *mDevice);

    mVertexBuffer = std::make_unique<VertexBuffer>(mGeometry->vertices(), *mCommandBuffers, *mDevice);
    mIndexBuffer = std::make_unique<IndexBuffer>(mGeometry->indices(), *mCommandBuffers, *mDevice);

    mUniformBuffers.resize(2);
    for (size_t i = 0; i < 2; i++)
    {
        mUniformBuffers[i] = std::make_unique<UniformBuffer>(*mDevice);
        updateUniformBuffer(i);
    }

    mDescriptorSets = std::make_unique<DescriptorSets>(bindings, mDescriptorSetLayout, *mDevice);
    for (size_t i = 0; i < 2; i++)
    {
        vk::DescriptorBufferInfo ubo_info;
        ubo_info.setBuffer(mUniformBuffers[i]->handle());
        ubo_info.setOffset(0);
        ubo_info.setRange(sizeof(UniformBufferObject));
        mDescriptorSets->update_descriptor_set(i, 0, ubo_info);
    }
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

    updateUniformBuffer(mCurrentFrame);

    res = hDevice.resetFences(1, &wait);
    auto cmd_buffer = mCommandBuffers->get_buffer(mCurrentFrame);
    cmd_buffer.reset();

#pragma region render_pass_command
    vk::CommandBufferBeginInfo begin_info;
    res = cmd_buffer.begin(&begin_info);
    vk::Rect2D render_area;
    render_area.setExtent(mSwapChain->extent());
    render_area.setOffset({0,0});

    std::array<vk::ClearValue, 2> clear_values {};
    clear_values[0].setColor(std::array<float, 4>{ 0.13f, 0.13f, 0.17f, 1.0f });
    clear_values[1].setDepthStencil({1.0f, 0 });

    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.setRenderPass(mRenderPass->handle());
    render_pass_info.setFramebuffer(mSwapChain->framebuffer(mCurrentFrame));
    render_pass_info.setRenderArea(render_area);
    render_pass_info.setClearValueCount(clear_values.size());
    render_pass_info.setPClearValues(clear_values.data());

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
    cmd_buffer.bindVertexBuffers(1, 1, &mInstanceBuffer->handle().handle(), offsets.data());
    cmd_buffer.bindIndexBuffer(mIndexBuffer->handle().handle(), 0, vk::IndexType::eUint32);
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  mPipelineLayout,
                                  0,
                                  1,
                                  &mDescriptorSets->get_set(mCurrentFrame),
                                  0,
                                  nullptr);

    cmd_buffer.drawIndexed(mIndexBuffer->index_count(), 8128, 0, 0,0);
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

    const auto extensions = mRequiredDeviceExtensions;
    mDevice = std::make_unique<Device>(*mSurface, *result, extensions);
}

void Application::selectDevice()
{
    const auto& devices = mInstance->physicalDevices();
    auto result = devices[0];
    mDevice = std::make_unique<Device>(*mSurface, result, mDefaultExtensions);
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

void Application::createGraphicsPipeline(const std::string& vert_shader_source, const std::string& frag_shader_source)
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
    create_info.setSetLayoutCount(1);
    create_info.setPSetLayouts(&mDescriptorSetLayout);

    auto result = mDevice->handle().createPipelineLayout(&create_info, nullptr, &mPipelineLayout);

    vk::VertexInputBindingDescription instance_binding;
    instance_binding.setBinding(1);
    instance_binding.setStride(sizeof(InstanceData));
    instance_binding.setInputRate(vk::VertexInputRate::eInstance);

    vk::VertexInputAttributeDescription instance_attrib;
    instance_attrib.setLocation(4);
    instance_attrib.setFormat(vk::Format::eR32G32B32A32Sfloat);
    instance_attrib.setBinding(1);
    instance_attrib.setOffset(0);

    GraphicsPipelineState pipeline_state;
    pipeline_state.add_binding_descriptions({
        { 0, sizeof(Vertex), vk::VertexInputRate::eVertex },
        { 1, sizeof(InstanceData), vk::VertexInputRate::eInstance }
    });
    pipeline_state.add_attribute_descriptions({
        { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
        { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) },
        { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) },
        { 3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv) },
        { 4, 1, vk::Format::eR32G32B32Sfloat, 0 },
        { 5, 1, vk::Format::eR32G32B32Sfloat, offsetof(InstanceData, color) },
        { 6, 1, vk::Format::eR32G32B32Sfloat, offsetof(InstanceData, scale) }
    });
    pipeline_state.add_scissor(scissor);
    pipeline_state.add_viewport(viewport);

    GraphicsPipelineBuilder builder(*mDevice, mPipelineLayout, *mRenderPass, pipeline_state);
    builder.add_shader(*vert_shader);
    builder.add_shader(*frag_shader);

    mGraphicsPipeline = builder.create_pipeline();
}

void Application::updateUniformBuffer(size_t index)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    auto model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1, 1, 0));
    auto view = glm::lookAt(glm::vec3(450, 0, 450), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
    auto proj = glm::perspective(glm::radians(45.0f), mSwapChain->aspectRatio(), 0.1f, 1000.0f);

    UniformBufferObject ubo {
        .view_projection = proj * view,
        .model = model
    };

    mUniformBuffers[index]->update(ubo);
}

void Application::cleanup()
{
    mSwapChain->destroy();

    Debug::destroy_vk_debug_msgr_ext(static_cast<VkInstance>(mInstance->handle()),
                                     mDebugMessenger,
                                     nullptr);
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
