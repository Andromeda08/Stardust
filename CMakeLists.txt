cmake_minimum_required(VERSION 3.23)
project(VkRaytracing)

set(CMAKE_CXX_STANDARD 20)

if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(PYTHON "py")
endif()

IF(APPLE)
    set(CMAKE_CXX_STANDARD 17)
    set(PYTHON "python3")

    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
ENDIF()

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)

# Create executable
add_executable(${PROJECT_NAME}
        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h
        Source/main.cpp
        # Utilities
        Source/Utility/Macro.hpp
        Source/Utility/Math.hpp Source/Utility/Math.cpp
        Source/Vulkan/DebugMessenger.hpp
        # Application
        Source/Struct/ApplicationSettings.hpp
        Source/Application.cpp Source/Application.hpp
        # Resources
        Source/Resources/Geometry.hpp
        Source/Resources/UniformBufferObject.hpp
        Source/Resources/Vertex.hpp
        # GLFW Window
        Source/Window.cpp Source/Window.cpp
        Source/Struct/WindowSettings.hpp
        # Vulkan Instance and Logical device
        Source/Vulkan/Instance.cpp Source/Vulkan/Instance.hpp
        Source/Vulkan/Device.cpp Source/Vulkan/Device.hpp
        # Images
        Source/Vulkan/Image/ImageView.cpp Source/Vulkan/Image/ImageView.hpp
        Source/Vulkan/Image/Image.cpp Source/Vulkan/Image/Image.hpp
        # Presentation
        Source/Vulkan/Surface.cpp Source/Vulkan/Surface.hpp
        Source/Vulkan/Swapchain.cpp Source/Vulkan/Swapchain.hpp
        # Synchronization objects
        Source/Vulkan/Synchronization/Semaphore.cpp Source/Vulkan/Synchronization/Semaphore.hpp
        Source/Vulkan/Synchronization/Fence.cpp Source/Vulkan/Synchronization/Fence.hpp
        # Commands
        Source/Vulkan/Command/CommandPool.cpp Source/Vulkan/Command/CommandPool.hpp
        Source/Vulkan/Command/CommandBuffers.cpp Source/Vulkan/Command/CommandBuffer.hpp
        # Descriptors
        Source/Vulkan/Descriptor/DescriptorSets.cpp Source/Vulkan/Descriptor/DescriptorSets.hpp
        Source/Vulkan/Descriptor/DescriptorSetLayout.hpp Source/Vulkan/Descriptor/DescriptorSetLayout.cpp
        # Graphics Pipeline
        Source/Vulkan/GraphicsPipeline/ShaderModule.cpp Source/Vulkan/GraphicsPipeline/ShaderModule.hpp
        Source/Vulkan/GraphicsPipeline/RenderPass.cpp Source/Vulkan/GraphicsPipeline/RenderPass.hpp
        Source/Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.cpp Source/Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp
        Source/Vulkan/GraphicsPipeline/GraphicsPipelineState.hpp
        Source/Vulkan/DepthBuffer.cpp Source/Vulkan/DepthBuffer.hpp

        Source/vk/Buffer.hpp
        Source/vk/VertexData.hpp
        Source/vk/UniformData.hpp
        Source/vk/InstanceData.hpp
        Source/vk/InstancedGeometry.hpp
        Source/vk/Mesh.hpp
        Source/vk/Scene.hpp
        Source/vk/Texture.hpp
        Source/vk/Image.hpp
        Source/vk/Sampler.hpp
        Source/vk/RayTracingScene.hpp

        Source/rt/AccelerationStructure.cpp
        Source/rt/AccelerationStructure.hpp
        Source/rt/RtAccelerator.hpp)

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