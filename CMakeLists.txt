cmake_minimum_required(VERSION 3.23)
project(Stardust)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(PYTHON "py")

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)
add_subdirectory(ThirdParty/json)

# Create executable
add_executable(${PROJECT_NAME}
        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h
        ThirdParty/spv-reflect/spirv_reflect.cpp ThirdParty/spv-reflect/spirv_reflect.h

        Old/main.cpp

        Old/Utility/Macro.hpp
        Old/Utility/Clock.hpp
        Old/Utility/Math.hpp Old/Utility/Math.cpp

        Old/Struct/ApplicationSettings.hpp
        Old/Struct/WindowSettings.hpp

        Old/Application.cpp Old/Application.hpp
        Old/Window.cpp Old/Window.cpp

        Old/vk/Buffer.hpp
        Old/vk/Geometry.hpp
        Old/vk/Mesh.hpp
        Old/vk/InstancedGeometry.hpp

        Old/vk/Material.hpp
        Old/vk/InstanceData.hpp
        Old/vk/UniformData.hpp
        Old/vk/VertexData.hpp

        Old/vk/Image.hpp Old/vk/Image.cpp
        Old/vk/Sampler.hpp Old/vk/Sampler.cpp
        Old/vk/Shader.hpp Old/vk/Shader.cpp
        Old/vk/Texture.hpp Old/vk/Texture.cpp

        Old/vk/Commands/CommandPool.cpp Old/vk/Commands/CommandPool.hpp
        Old/vk/Commands/CommandBuffers.cpp Old/vk/Commands/CommandBuffers.hpp

        Old/vk/Descriptors/DescriptorWrites.hpp Old/vk/Descriptors/DescriptorWrites.cpp
        Old/vk/Descriptors/DescriptorSetLayout.hpp Old/vk/Descriptors/DescriptorSetLayout.cpp
        Old/vk/Descriptors/DescriptorSets.cpp Old/vk/Descriptors/DescriptorSets.hpp

        Old/vk/Device/DebugMessenger.hpp
        Old/vk/Device/Instance.cpp Old/vk/Device/Instance.hpp
        Old/vk/Device/Device.cpp Old/vk/Device/Device.hpp
        Old/vk/Device/Surface.cpp Old/vk/Device/Surface.hpp

        Old/vk/Pipelines/Pipeline.hpp
        Old/vk/Pipelines/PipelineBuilder.cpp Old/vk/Pipelines/PipelineBuilder.hpp
        Old/vk/Pipelines/PipelineBuilder.hpp Old/vk/Pipelines/PipelineBuilder.cpp
        Old/vk/Pipelines/PipelineState.hpp Old/vk/Pipelines/PipelineState.cpp
        Old/vk/Pipelines/RenderPass.cpp Old/vk/Pipelines/RenderPass.hpp

        Old/vk/Presentation/Swapchain.cpp Old/vk/Presentation/Swapchain.hpp
        Old/vk/Presentation/SwapchainCapabilities.hpp

        Old/vk/Synchronization/Semaphore.cpp Old/vk/Synchronization/Semaphore.hpp
        Old/vk/Synchronization/Fence.cpp Old/vk/Synchronization/Fence.hpp

        Old/rt/AccelerationStructure.hpp Old/rt/AccelerationStructure.cpp
        Old/rt/RtAccelerator.hpp Old/rt/RtAccelerator.cpp

        Old/Scenes/SceneManager.hpp Old/Scenes/SceneManager.cpp
        Old/Scenes/Camera.hpp Old/Scenes/Camera.cpp
        Old/Scenes/Scene.cpp Old/Scenes/Scene.hpp
        Old/Scenes/Voxels.hpp Old/Scenes/Voxels.cpp
        Old/Scenes/Raytracing.cpp Old/Scenes/Raytracing.hpp)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Link to libraries and add include directories
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Old
        ThirdParty/spv-reflect
        ThirdParty/stb)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${PYTHON} compile_shaders.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

set(V2 "Stardust2")
add_executable(${V2}
        Stardust/main.cpp

        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h

        Stardust/Window/Window.hpp Stardust/Window/Window.cpp Stardust/Window/WindowOptions.hpp
        Stardust/Application/Application.hpp Stardust/Application/Application.cpp Stardust/Application/ApplicationOptions.hpp

        Stardust/Resources/CameraUniformData.hpp
        Stardust/Resources/Geometry.hpp
        Stardust/Resources/VertexData.hpp
        Stardust/Resources/Primitives/Cube.hpp
        Stardust/Resources/Primitives/Sphere.hpp

        Stardust/Vulkan/Buffer.hpp Stardust/Vulkan/Buffer.cpp
        Stardust/Vulkan/Context.cpp Stardust/Vulkan/Context.hpp
        Stardust/Vulkan/ContextBuilder.cpp Stardust/Vulkan/ContextBuilder.hpp Stardust/Vulkan/ContextOptions.hpp
        Stardust/Vulkan/CommandBuffers.cpp Stardust/Vulkan/CommandBuffers.hpp
        Stardust/Vulkan/DeviceFeatures.hpp
        Stardust/Vulkan/Utils.hpp
        Stardust/Vulkan/Queues.hpp

        Stardust/Vulkan/Descriptors/Descriptor.cpp Stardust/Vulkan/Descriptors/Descriptor.hpp
        Stardust/Vulkan/Descriptors/DescriptorBuilder.hpp Stardust/Vulkan/Descriptors/DescriptorBuilder.cpp
        Stardust/Vulkan/Descriptors/DescriptorWrites.cpp Stardust/Vulkan/Descriptors/DescriptorWrites.hpp

        Stardust/Vulkan/Image/Sampler.hpp
        Stardust/Vulkan/Image/Image.cpp Stardust/Vulkan/Image/Image.hpp

        Stardust/Vulkan/Rendering/ShaderModule.cpp Stardust/Vulkan/Rendering/ShaderModule.hpp
        Stardust/Vulkan/Rendering/Mesh.cpp Stardust/Vulkan/Rendering/Mesh.hpp
        Stardust/Vulkan/Rendering/Pipeline.hpp
        Stardust/Vulkan/Rendering/PipelineBuilder.cpp Stardust/Vulkan/Rendering/PipelineBuilder.hpp
        Stardust/Vulkan/Rendering/PipelineState.cpp Stardust/Vulkan/Rendering/PipelineState.hpp
        Stardust/Vulkan/Rendering/RenderPass.hpp Stardust/Vulkan/Rendering/RenderPass.cpp
        Stardust/Vulkan/RendererOptions.hpp

        Stardust/Vulkan/Presentation/Swapchain.cpp Stardust/Vulkan/Presentation/Swapchain.hpp
        Stardust/Vulkan/Presentation/SwapchainBuilder.hpp Stardust/Vulkan/Presentation/SwapchainBuilder.cpp
        Stardust/Vulkan/Presentation/SwapchainCapabilities.hpp

        Stardust/Vulkan/Image/DepthBuffer.cpp Stardust/Vulkan/Image/DepthBuffer.hpp
        Stardust/Vulkan/Image/Texture.cpp Stardust/Vulkan/Image/Texture.hpp

        Stardust/Vulkan/Raytracing/Blas.cpp Stardust/Vulkan/Raytracing/Blas.hpp
        Stardust/Vulkan/Raytracing/Tlas.cpp Stardust/Vulkan/Raytracing/Tlas.hpp

        Stardust/Scene/Scene.hpp Stardust/Scene/Scene.cpp
        Stardust/Scene/Transform.hpp
        Stardust/Scene/Object.hpp
        Stardust/Scene/Light.hpp
        Stardust/Scene/Camera.cpp Stardust/Scene/Camera.hpp)

target_compile_definitions(${V2} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 SD_DEBUG USE_NSIGHT_AFTERMATH=1)
target_link_libraries(${V2} ${Vulkan_LIBRARIES} glfw glm nlohmann_json::nlohmann_json)
target_include_directories(${V2} PUBLIC ${Vulkan_INCLUDE_DIRS} Stardust ThirdParty/stb ThirdParty/aftermath/include)