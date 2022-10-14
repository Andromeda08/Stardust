cmake_minimum_required(VERSION 3.23)
project(VkRaytracing)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)

# Create executable
add_executable(${PROJECT_NAME}
        Source/main.cpp
        # Utilities
        Source/Core/Macro.hpp
        Source/Core/Vulkan/ThirdParty/Vma.cpp
        Source/Core/Vulkan/DebugMessenger.hpp
        # Application
        Source/Core/Struct/ApplicationSettings.hpp
        Source/Core/Application.cpp Source/Core/Application.hpp
        # Resources
        Source/Core/Resources/UniformBufferObject.hpp
        Source/Core/Resources/Vertex.hpp
        # GLFW Window
        Source/Core/Window.cpp Source/Core/Window.cpp
        Source/Core/Struct/WindowSettings.hpp
        # Vulkan Instance and Logical device
        Source/Core/Vulkan/Instance.cpp Source/Core/Vulkan/Instance.hpp
        Source/Core/Vulkan/Device.cpp Source/Core/Vulkan/Device.hpp
        # Images
        Source/Core/Vulkan/Image/ImageView.cpp Source/Core/Vulkan/Image/ImageView.hpp
        Source/Core/Vulkan/Image/Image.cpp Source/Core/Vulkan/Image/Image.hpp
        # Presentation
        Source/Core/Vulkan/Surface.cpp Source/Core/Vulkan/Surface.hpp
        Source/Core/Vulkan/Swapchain.cpp Source/Core/Vulkan/Swapchain.hpp
        # Synchronization objects
        Source/Core/Vulkan/Synchronization/Semaphore.cpp Source/Core/Vulkan/Synchronization/Semaphore.hpp
        Source/Core/Vulkan/Synchronization/Fence.cpp Source/Core/Vulkan/Synchronization/Fence.hpp
        # Commands
        Source/Core/Vulkan/Command/CommandPool.cpp Source/Core/Vulkan/Command/CommandPool.hpp
        Source/Core/Vulkan/Command/CommandBuffers.cpp Source/Core/Vulkan/Command/CommandBuffers.hpp
        # Buffers
        Source/Core/Vulkan/Buffer/Buffer.cpp Source/Core/Vulkan/Buffer/Buffer.hpp
        Source/Core/Vulkan/Buffer/VertexBuffer.cpp Source/Core/Vulkan/Buffer/VertexBuffer.hpp
        Source/Core/Vulkan/Buffer/IndexBuffer.cpp Source/Core/Vulkan/Buffer/IndexBuffer.hpp
        Source/Core/Vulkan/Buffer/UniformBuffer.cpp Source/Core/Vulkan/Buffer/UniformBuffer.hpp
        # Descriptors
        Source/Core/Vulkan/Descriptor/DescriptorSets.cpp Source/Core/Vulkan/Descriptor/DescriptorSets.hpp
        # Graphics Pipeline
        Source/Core/Vulkan/GraphicsPipeline/ShaderModule.cpp Source/Core/Vulkan/GraphicsPipeline/ShaderModule.hpp
        Source/Core/Vulkan/GraphicsPipeline/RenderPass.cpp Source/Core/Vulkan/GraphicsPipeline/RenderPass.hpp
        Source/Core/Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.cpp Source/Core/Vulkan/GraphicsPipeline/GraphicsPipelineBuilder.hpp
        Source/Core/Vulkan/GraphicsPipeline/GraphicsPipelineState.hpp
        Source/Core/Vulkan/DepthBuffer.cpp Source/Core/Vulkan/DepthBuffer.hpp)

# Link to libraries and add include directories
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
