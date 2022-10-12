cmake_minimum_required(VERSION 3.23)
project(VkRaytracing)

set(CMAKE_CXX_STANDARD 20)

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/glm)

# Create executable
add_executable(${PROJECT_NAME}
        Source/main.cpp
        # Utilities
        Source/Core/Macro.hpp
        # Application
        Source/Core/Struct/ApplicationSettings.hpp
        Source/Core/Application.cpp Source/Core/Application.hpp
        # GLFW Window
        Source/Core/Window.cpp Source/Core/Window.cpp
        Source/Core/Struct/WindowSettings.hpp
        # Vulkan Instance and Logical device
        Source/Core/Vulkan/Instance.cpp Source/Core/Vulkan/Instance.hpp
        Source/Core/Vulkan/Device.cpp Source/Core/Vulkan/Device.hpp
        # Presentation
        Source/Core/Vulkan/ImageView.cpp Source/Core/Vulkan/ImageView.hpp
        Source/Core/Vulkan/Surface.cpp Source/Core/Vulkan/Surface.hpp
        Source/Core/Vulkan/Swapchain.cpp Source/Core/Vulkan/Swapchain.hpp
        # Synchronization objects
        Source/Core/Vulkan/Synchronization/Semaphore.cpp Source/Core/Vulkan/Synchronization/Semaphore.hpp
        Source/Core/Vulkan/Synchronization/Fence.cpp Source/Core/Vulkan/Synchronization/Fence.hpp Source/Core/Vulkan/Command/CommandPool.cpp Source/Core/Vulkan/Command/CommandPool.hpp Source/Core/Vulkan/Command/CommandBuffers.cpp Source/Core/Vulkan/Command/CommandBuffers.hpp)

# Link to libraries and add include directories
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
