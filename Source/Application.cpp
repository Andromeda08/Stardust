#include "Application.hpp"
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
#include "Vulkan/Descriptor/DescriptorSetLayout.hpp"

Application::Application(const ApplicationSettings& app_settings)
: mSettings(app_settings)
, mCurrentFrame(0)
{
#if defined(__APPLE__)
    // Yeah, we're not going to do this any time soon.
    mSettings.raytracing = false;
#endif

    mWindow   = std::make_unique<Window>(app_settings.windowSettings);

    mInstance = std::make_unique<Instance>(*mWindow);

    setupDebugMessenger();

    mSurface  = std::make_unique<Surface>(*mInstance);

    (mSettings.raytracing) ? selectRayTracingDevice() : selectDevice();

    createSwapChain();

    mCommandBuffers = std::make_unique<CommandBuffer>(*mDevice);

    mDepthBuffer = std::make_unique<DepthBuffer>(mSwapChain->extent(), *mDevice, *mCommandBuffers);

    mRenderPass = std::make_unique<RenderPass>(*mDevice, mSwapChain->format(), mDepthBuffer->format());

    mSwapChain->createFrameBuffers(*mRenderPass, *mDepthBuffer);

    auto ubo_binding = re::UniformBuffer<re::UniformData>::make_binding(0);
    auto sampler_binding = re::Sampler::make_binding(1);

    std::vector<vk::DescriptorSetLayoutBinding> bindings = { ubo_binding, sampler_binding };

    vk::DescriptorSetLayoutCreateInfo layout_create_info;
    layout_create_info.setBindingCount(bindings.size());
    layout_create_info.setPBindings(bindings.data());
    auto result = mDevice->handle().createDescriptorSetLayout(&layout_create_info, nullptr, &mDescriptorSetLayout);

    mUniformBuffers.resize(2);
    for (size_t i = 0; i < 2; i++)
    {
        mUniformBuffers[i] = std::make_unique<re::UniformBuffer<re::UniformData>>(*mCommandBuffers);
        updateUniformBuffer(i);
    }

    mTexture2D = std::make_unique<re::Texture>("mirza_vulkan.jpg", *mCommandBuffers);
    mSampler2D = std::make_unique<re::Sampler>(*mDevice);

    mDescriptorSets = std::make_unique<DescriptorSets>(bindings, mDescriptorSetLayout, *mDevice);
    for (size_t i = 0; i < 2; i++)
    {
        vk::DescriptorBufferInfo ubo_info;
        ubo_info.setBuffer(mUniformBuffers[i]->buffer());
        ubo_info.setOffset(0);
        ubo_info.setRange(sizeof(re::UniformData));
        mDescriptorSets->update_descriptor_set(i, 0, ubo_info);

        vk::DescriptorImageInfo sampler_info;
        sampler_info.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        sampler_info.setImageView(mTexture2D->view());
        sampler_info.setSampler(mSampler2D->sampler());
        mDescriptorSets->update_descriptor_set(i, 1, sampler_info);
    }

    mGraphicsPipeline = createGraphicsPipeline("phong.vert.spv", "tex.frag.spv");

#if !defined(__APPLE__)
    mReScene = std::make_unique<re::RayTracingScene>(*mSwapChain, *mCommandBuffers);
#else
    mReScene = std::make_unique<re::Scene>(*mCommandBuffers);
#endif
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow->handle()))
    {
        glfwPollEvents();

        draw();

        mDevice->waitIdle();

        //glfwSetWindowShouldClose(mWindow->handle(), true);
    }

    Application::cleanup();
}

void Application::draw()
{
    auto hDevice = mDevice->handle();

    auto wait = mInFlightFences[mCurrentFrame].handle();
    auto res = hDevice.waitForFences(1, &wait, true, UINT64_MAX);
    res = hDevice.resetFences(1, &wait);

    vk::Semaphore semaphore = mImageAvailableSemaphores[mCurrentFrame].handle();
    auto result = hDevice.acquireNextImageKHR(mSwapChain->handle(), UINT64_MAX, semaphore, nullptr);

    res = hDevice.resetFences(1, &wait);
    auto cmd_buffer = mCommandBuffers->get_buffer(mCurrentFrame);
    cmd_buffer.reset();

    vk::CommandBufferBeginInfo begin_info;
    res = cmd_buffer.begin(&begin_info);

    /*
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

    auto viewport = mSwapChain->make_viewport();
    cmd_buffer.setViewport(0, 1, &viewport);

    auto scissor = mSwapChain->make_scissor();
    cmd_buffer.setScissor(0, 1, &scissor);

    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 0, 1,
                                  &mDescriptorSets->get_set(mCurrentFrame), 0, nullptr);

    updateUniformBuffer(mCurrentFrame);
    mReScene->rasterize(cmd_buffer);
    cmd_buffer.endRenderPass();
     */

#if !defined(__APPLE__)
    re::vkImage::image_barrier(mReScene->output(),
                               {}, vk::AccessFlagBits::eShaderWrite,
                               vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                               vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                               *mCommandBuffers);

    mReScene->trace_rays(mCurrentFrame, cmd_buffer);
#endif

    cmd_buffer.end();

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

    re::vkImage::image_barrier(mReScene->output(),
                               vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                               vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal,
                               vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eComputeShader,
                               *mCommandBuffers);

    re::vkImage::image_barrier(mSwapChain->image(mCurrentFrame),
                               {}, vk::AccessFlagBits::eShaderWrite,
                               vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                               vk::PipelineStageFlagBits::eNone, vk::PipelineStageFlagBits::eComputeShader,
                               *mCommandBuffers);

    auto cmd1 = mCommandBuffers->begin_single_time();
    mReScene->copy_to_swapchain(mCurrentFrame, cmd1);
    mCommandBuffers->end_single_time(cmd1);

    re::vkImage::image_barrier(mSwapChain->image(mCurrentFrame),
                               vk::AccessFlagBits::eShaderWrite, {},
                               vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR,
                               vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eNone,
                               *mCommandBuffers);

    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphoreCount(1);
    present_info.setPWaitSemaphores(signal_semaphores);
    present_info.setSwapchainCount(1);
    present_info.setPSwapchains(&mSwapChain->handle());
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

    mDevice = std::make_unique<Device>(*mInstance, *mSurface, result, mDefaultExtensions);
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

vk::Pipeline Application::createGraphicsPipeline(const std::string& vert_shader_source, const std::string& frag_shader_source)
{
    auto vert_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eVertex,
                                                        vert_shader_source,
                                                        *mDevice);

    auto frag_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eFragment,
                                                        frag_shader_source,
                                                        *mDevice);

    auto viewport = mSwapChain->make_viewport();
    auto scissor = mSwapChain->make_scissor();

    vk::PipelineLayoutCreateInfo create_info;
    create_info.setSetLayoutCount(1);
    create_info.setPSetLayouts(&mDescriptorSetLayout);

    auto result = mDevice->handle().createPipelineLayout(&create_info, nullptr, &mPipelineLayout);

    GraphicsPipelineState pipeline_state;
    pipeline_state.add_binding_descriptions({
        { 0, sizeof(re::VertexData), vk::VertexInputRate::eVertex },
        { 1, sizeof(re::InstanceData), vk::VertexInputRate::eInstance }
    });
    pipeline_state.add_attribute_descriptions({
        { 0, 0, vk::Format::eR32G32B32Sfloat, 0 },
        { 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(re::VertexData, color) },
        { 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(re::VertexData, normal) },
        { 3, 0, vk::Format::eR32G32Sfloat, offsetof(re::VertexData, uv) },
        { 4, 1, vk::Format::eR32G32B32Sfloat, 0 },
        { 5, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, scale) },
        { 6, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, r_axis) },
        { 7, 1, vk::Format::eR32Sfloat, offsetof(re::InstanceData, r_angle) },
        { 8, 1, vk::Format::eR32G32B32Sfloat, offsetof(re::InstanceData, color) }
    });
    pipeline_state.add_scissor(scissor);
    pipeline_state.add_viewport(viewport);

    GraphicsPipelineBuilder builder(*mDevice, mPipelineLayout, *mRenderPass, pipeline_state);
    builder.add_shader(*vert_shader);
    builder.add_shader(*frag_shader);

    return builder.create_pipeline();
}

void Application::updateUniformBuffer(size_t index)
{
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    auto model = glm::mat4(1.0f); // glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1, 1, 0));
    auto view = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
    auto proj = glm::perspective(glm::radians(45.0f), mSwapChain->aspectRatio(), 0.1f, 2000.0f);

    re::UniformData ubo {
        .view_projection = proj * view,
        .model = model,
        .time = time
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
