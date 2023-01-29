cmake_minimum_required(VERSION 3.23)
project(Stardust)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(PYTHON "py")

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)

# Create executable
add_executable(${PROJECT_NAME}
        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h
        ThirdParty/spv-reflect/spirv_reflect.cpp ThirdParty/spv-reflect/spirv_reflect.h

        Source/main.cpp

        Source/Utility/Macro.hpp
        Source/Utility/Clock.hpp
        Source/Utility/Math.hpp Source/Utility/Math.cpp

        Source/Struct/ApplicationSettings.hpp
        Source/Struct/WindowSettings.hpp

        Source/Application.cpp Source/Application.hpp
        Source/Window.cpp Source/Window.cpp

        Source/vk/Buffer.hpp
        Source/vk/Geometry.hpp
        Source/vk/Mesh.hpp
        Source/vk/InstancedGeometry.hpp

        Source/vk/Material.hpp
        Source/vk/InstanceData.hpp
        Source/vk/UniformData.hpp
        Source/vk/VertexData.hpp

        Source/vk/Image.hpp   Source/vk/Image.cpp
        Source/vk/Sampler.hpp Source/vk/Sampler.cpp
        Source/vk/Shader.hpp  Source/vk/Shader.cpp
        Source/vk/Texture.hpp Source/vk/Texture.cpp

        Source/vk/Commands/CommandPool.cpp    Source/vk/Commands/CommandPool.hpp
        Source/vk/Commands/CommandBuffers.cpp Source/vk/Commands/CommandBuffers.hpp

        Source/vk/Descriptors/DescriptorWrites.hpp    Source/vk/Descriptors/DescriptorWrites.cpp
        Source/vk/Descriptors/DescriptorSetLayout.hpp Source/vk/Descriptors/DescriptorSetLayout.cpp
        Source/vk/Descriptors/DescriptorSets.cpp      Source/vk/Descriptors/DescriptorSets.hpp

        Source/vk/Device/DebugMessenger.hpp
        Source/vk/Device/Instance.cpp       Source/vk/Device/Instance.hpp
        Source/vk/Device/Device.cpp         Source/vk/Device/Device.hpp
        Source/vk/Device/Surface.cpp        Source/vk/Device/Surface.hpp

        Source/vk/Pipelines/Pipeline.hpp
        Source/vk/Pipelines/PipelineBuilder.cpp Source/vk/Pipelines/PipelineBuilder.hpp
        Source/vk/Pipelines/PipelineBuilder.hpp Source/vk/Pipelines/PipelineBuilder.cpp
        Source/vk/Pipelines/PipelineState.hpp   Source/vk/Pipelines/PipelineState.cpp
        Source/vk/Pipelines/RenderPass.cpp      Source/vk/Pipelines/RenderPass.hpp

        Source/vk/Presentation/Swapchain.cpp Source/vk/Presentation/Swapchain.hpp
        Source/vk/Presentation/SwapchainCapabilities.hpp

        Source/vk/Synchronization/Semaphore.cpp Source/vk/Synchronization/Semaphore.hpp
        Source/vk/Synchronization/Fence.cpp     Source/vk/Synchronization/Fence.hpp

        Source/rt/AccelerationStructure.hpp Source/rt/AccelerationStructure.cpp
        Source/rt/RtAccelerator.hpp         Source/rt/RtAccelerator.cpp

        Source/Scenes/SceneManager.hpp Source/Scenes/SceneManager.cpp
        Source/Scenes/Camera.hpp Source/Scenes/Camera.cpp
        Source/Scenes/Scene.cpp Source/Scenes/Scene.hpp
        Source/Scenes/Voxels.hpp Source/Scenes/Voxels.cpp
        Source/Scenes/Raytracing.cpp Source/Scenes/Raytracing.hpp)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Link to libraries and add include directories
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
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
        Stardust/Vulkan/Rendering/RenderPass.cpp Stardust/Vulkan/Rendering/RenderPass.hpp

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
        Stardust/Scene/Light.hpp)

target_compile_definitions(${V2} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 SD_DEBUG)
target_link_libraries(${V2} ${Vulkan_LIBRARIES} glfw glm)
target_include_directories(${V2} PUBLIC ${Vulkan_INCLUDE_DIRS} Stardust ThirdParty/stb)