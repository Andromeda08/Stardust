cmake_minimum_required(VERSION 3.23)
project(Stardust)

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