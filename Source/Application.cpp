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

constexpr uint32_t g_instance_count = 8128;

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

    mGraphicsPipeline = createGraphicsPipeline("phong.vert.spv", "phong.frag.spv");

    mReScene = std::make_unique<re::Scene>(*mCommandBuffers);

#pragma region raytracing_setup
#if defined(__APPLE__)
    if (mSettings.raytracing)
    {
        auto rt_start = std::chrono::high_resolution_clock::now();

        auto sphere = std::make_unique<SphereGeometry>(1.0f, glm::vec3{0.5f, 0.5f, 0.5f}, 60);
        auto test_blas = BlasInfo::create_blas(rt_mesh, *mCommandBuffers, mDevice->dispatch());

        std::vector<vk::TransformMatrixKHR> transforms;
        std::vector<vk::AccelerationStructureInstanceKHR> instances(g_instance_count / 2);

        for (size_t i = 0; i < instances.size(); i++)
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::scale(m, v[i].scale);
            m = glm::translate(m, v[i].translate);

            auto model = Math::model(v[i].translate, v[i].scale);

            transforms.push_back(Math::glmToKhr(model));

            instances[i].setInstanceCustomIndex(i);
            instances[i].setTransform(transforms[i]);
            instances[i].setMask(0xff);
            instances[i].setInstanceShaderBindingTableRecordOffset(0);
            instances[i].setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
            instances[i].setAccelerationStructureReference(test_blas.buffer->address());
        }

        vk::DeviceSize ibsize = instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        auto staging = Buffer::make_staging_buffer(ibsize, *mDevice);

        void *data;
        vkMapMemory(mDevice->handle(), staging.memory(), 0, ibsize, 0, &data);
        memcpy(data, instances.data(), (size_t) ibsize);
        vkUnmapMemory(mDevice->handle(), staging.memory());

        auto instance_buffer = std::make_unique<Buffer>(ibsize,
                                                        vk::BufferUsageFlagBits::eTransferDst
                                                        | vk::BufferUsageFlagBits::eShaderDeviceAddress
                                                        | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
                                                        vk::MemoryPropertyFlagBits::eDeviceLocal
                                                        | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                        *mDevice);

        Buffer::copy_buffer(*mCommandBuffers, staging.handle(), instance_buffer->handle(), ibsize);


        auto test_tlas = TlasInfo::create_tlas(instances.size(),
                                               instance_buffer->address(),
                                               *mCommandBuffers);

        auto rt_end = std::chrono::high_resolution_clock::now();
        auto duration = duration_cast<std::chrono::milliseconds>(rt_end - rt_start);
        std::cout << "Acceleration Structure build time: " << duration.count() << "ms\n"
            << "\tBottom Level : " << test_mesh.vertex_buffer->vertex_count() << " Vertices\n"
            << "\tTop Level    : " << instances.size() << " Instances" << std::endl;
    }
#endif
#pragma endregion
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
    auto res = hDevice.waitForFences(1, &wait, true, UINT64_MAX);
    res = hDevice.resetFences(1, &wait);

    vk::Semaphore semaphore = mImageAvailableSemaphores[mCurrentFrame].handle();
    auto result = hDevice.acquireNextImageKHR(mSwapChain->handle(), UINT64_MAX, semaphore, nullptr);

    res = hDevice.resetFences(1, &wait);
    auto cmd_buffer = mCommandBuffers->get_buffer(mCurrentFrame);
    cmd_buffer.reset();

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

    auto viewport = mSwapChain->make_viewport();
    cmd_buffer.setViewport(0, 1, &viewport);

    auto scissor = mSwapChain->make_scissor();
    cmd_buffer.setScissor(0, 1, &scissor);

#pragma region render_items

    cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline);
    cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 0, 1,
                                  &mDescriptorSets->get_set(mCurrentFrame), 0, nullptr);

    updateUniformBuffer(mCurrentFrame);
    mReScene->draw(mCurrentFrame, cmd_buffer);

#pragma endregion

    cmd_buffer.endRenderPass();
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

    auto model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1, 1, 0));
    auto view = glm::lookAt(glm::vec3(128, 0, 128), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
    auto proj = glm::perspective(glm::radians(45.0f), mSwapChain->aspectRatio(), 0.1f, 2000.0f);

    re::UniformData ubo {
        .view_projection = proj * view,
        .model = model,
        .time = time
    };

    mUniformBuffers[index]->update(ubo);
}

template <typename T>
inline T round_up(T k, T alignment) {
    return (k + alignment - 1) & ~(alignment - 1);
}

#if defined(__APPLE__)
void Application::createRayTracingScene()
{
    vk::Result result;
    vk::PhysicalDeviceProperties2 pdp;
    pdp.pNext = &mRtProps;
    mDevice->physicalDevice().getProperties2(&pdp, mDevice->dispatch());

#pragma region create_mesh_and_instances
    size_t instance_count = 16;
    glm::vec2 pos { -50, 50 };
    glm::vec2 scale { 1.0f, 5.0f };

    Geometry* sphere = new SphereGeometry(1.0f, glm::vec3{0.5f, 0.5f, 0.5f}, 60);

    std::default_random_engine rand(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<int>   pos_mod(pos.x, pos.y);
    std::uniform_real_distribution<float> color_mod(0.0f, 1.0f);
    std::uniform_real_distribution<float> scale_mod(scale.x, scale.y);

    std::vector<IData> instance_data(instance_count);
    for (size_t i = 0; i < instance_count; i++)
    {
        /*instance_data.push_back({
            glm::vec3{ pos_mod(rand), pos_mod(rand), pos_mod(rand) },
            glm::vec3{ scale_mod(rand) },
            glm::vec3{ 1 },
            0.0f,
            glm::vec3{ color_mod(rand), color_mod(rand), color_mod(rand) }
        });*/
    }

    mRtMesh = std::make_unique<Mesh>(sphere, instance_data, *mCommandBuffers);
#pragma endregion

    //mRtAccelerator = RtAccelerator::create_accelerator(*mRtMesh, *mCommandBuffers);

    mRtDescriptorSetLayout = DescriptorSetLayout()
        .storage_image(0, vk::ShaderStageFlagBits::eRaygenKHR)
        .accelerator(1, vk::ShaderStageFlagBits::eRaygenKHR)
        .uniform_buffer(2, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
        .storage_buffer(3, vk::ShaderStageFlagBits::eClosestHitKHR)
        .storage_buffer(4, vk::ShaderStageFlagBits::eClosestHitKHR)
        .sampled_image(5, vk::ShaderStageFlagBits::eClosestHitKHR)
        .sampler(6, vk::ShaderStageFlagBits::eClosestHitKHR)
        .create(*mDevice);

#pragma region create_rt_pipeline
    std::vector<vk::PushConstantRange> push_constants = {
        { vk::ShaderStageFlagBits::eRaygenKHR, 0, 4 },
        { vk::ShaderStageFlagBits::eClosestHitKHR, 4, 4 }
    };

    vk::PipelineLayoutCreateInfo plci;
    plci.setSetLayoutCount(1);
    plci.setPSetLayouts(&mRtDescriptorSetLayout);
    plci.setPushConstantRangeCount(push_constants.size());
    plci.setPPushConstantRanges(push_constants.data());

    result = mDevice->handle().createPipelineLayout(&plci, nullptr, &mRtPipelineLayout);

    auto rgen_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eRaygenKHR,
                                                      "rt.rgen.spv", *mDevice);
    auto miss_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eMissKHR,
                                                      "rt.rmiss.spv", *mDevice);
    auto chit_shader = std::make_unique<ShaderModule>(vk::ShaderStageFlagBits::eClosestHitKHR,
                                                      "rt.rchit.spv", *mDevice);

    std::vector<vk::PipelineShaderStageCreateInfo> stage_infos(3);
#pragma region shader_stage_infos
    stage_infos[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR);
    stage_infos[0].setModule(rgen_shader->handle());
    stage_infos[0].setPName("main");

    stage_infos[1].setStage(vk::ShaderStageFlagBits::eMissKHR);
    stage_infos[1].setModule(miss_shader->handle());
    stage_infos[1].setPName("main");

    stage_infos[2].setStage(vk::ShaderStageFlagBits::eClosestHitKHR);
    stage_infos[2].setModule(chit_shader->handle());
    stage_infos[2].setPName("main");
#pragma endregion

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups(3);
#pragma region shader_groups
    shader_groups[0].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    shader_groups[0].setGeneralShader(0);
    shader_groups[0].setClosestHitShader(VK_SHADER_UNUSED_KHR);
    shader_groups[0].setAnyHitShader(VK_SHADER_UNUSED_KHR);
    shader_groups[0].setIntersectionShader(VK_SHADER_UNUSED_KHR);

    shader_groups[1].setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    shader_groups[1].setGeneralShader(1);
    shader_groups[1].setClosestHitShader(VK_SHADER_UNUSED_KHR);
    shader_groups[1].setAnyHitShader(VK_SHADER_UNUSED_KHR);
    shader_groups[1].setIntersectionShader(VK_SHADER_UNUSED_KHR);

    shader_groups[2].setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
    shader_groups[2].setGeneralShader(VK_SHADER_UNUSED_KHR);
    shader_groups[2].setClosestHitShader(VK_SHADER_UNUSED_KHR);
    shader_groups[2].setAnyHitShader(2);
    shader_groups[2].setIntersectionShader(VK_SHADER_UNUSED_KHR);
#pragma endregion

    vk::RayTracingPipelineCreateInfoKHR create_info;
    create_info.setFlags(vk::PipelineCreateFlagBits::eRayTracingNoNullClosestHitShadersKHR |
                         vk::PipelineCreateFlagBits::eRayTracingNoNullMissShadersKHR);
    create_info.setStageCount(stage_infos.size());
    create_info.setPStages(stage_infos.data());
    create_info.setGroupCount(shader_groups.size());
    create_info.setPGroups(shader_groups.data());
    create_info.setLayout(mRtPipelineLayout);

    result = mDevice->handle().createRayTracingPipelinesKHR(nullptr, nullptr, 1, &create_info, nullptr, &mRtPipeline, mDevice->dispatch());
#pragma endregion

    // TODO: Descriptor Writes

#pragma region create_sbt
    uint32_t miss_offset = round_up(mRtProps.shaderGroupHandleSize, mRtProps.shaderGroupBaseAlignment);
    uint32_t chit_offset = round_up(miss_offset + mRtProps.shaderGroupHandleSize, mRtProps.shaderGroupBaseAlignment);
    uint32_t sbt_size = chit_offset + mRtProps.shaderGroupHandleSize;

    std::vector<uint8_t> sbt_data(sbt_size);
    result = mDevice->handle().getRayTracingShaderGroupHandlesKHR(mRtPipeline, 0, 1, mRtProps.shaderGroupHandleSize, sbt_data.data() + 0, mDevice->dispatch());
    result = mDevice->handle().getRayTracingShaderGroupHandlesKHR(mRtPipeline, 1, 1, mRtProps.shaderGroupHandleSize, sbt_data.data() + miss_offset, mDevice->dispatch());
    result = mDevice->handle().getRayTracingShaderGroupHandlesKHR(mRtPipeline, 2, 1, mRtProps.shaderGroupHandleSize, sbt_data.data() + chit_offset, mDevice->dispatch());

    auto staging = Buffer::make_staging_buffer(sbt_size, *mDevice);

    void *data;
    vkMapMemory(mDevice->handle(), staging.memory(), 0, sbt_size, 0, &data);
    memcpy(data, sbt_data.data(), (size_t) sbt_size);
    vkUnmapMemory(mDevice->handle(), staging.memory());

    mRtShaderBindingTable = std::make_unique<Buffer>(sbt_size,
                                                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                     *mDevice, mDevice->dispatch());

    Buffer::copy_buffer(*mCommandBuffers, staging.handle(), mRtShaderBindingTable->handle(), sbt_size);
#pragma endregion
}
#endif

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
