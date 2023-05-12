cmake_minimum_required(VERSION 3.23)
project(Stardust)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_EXE_LINKER_FLAGS "-static")
set(PYTHON "py")

# Set up dependencies
find_package(Vulkan REQUIRED)

add_subdirectory(ThirdParty/glfw)
add_subdirectory(ThirdParty/json)

add_subdirectory(ThirdParty/glm)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(IMGUI_DIR ThirdParty/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

set(IMNODES_DIR ThirdParty/imnodes)
include_directories(${IMNODES_DIR})

add_executable(${PROJECT_NAME}
        Stardust/main.cpp

        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h
        ThirdParty/tinyobj/tiny_obj_impl.cpp
        ThirdParty/stduuid/uuid.h

        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp

        ${IMNODES_DIR}/imnodes.cpp

        Stardust/Window/Window.hpp Stardust/Window/Window.cpp
        Stardust/Window/WindowOptions.hpp

        Stardust/Application/Application.hpp Stardust/Application/Application.cpp
        Stardust/Application/ApplicationOptions.hpp
        Stardust/Application/Configuration.hpp

        Stardust/Benchmarking.hpp

        Stardust/Resources/CameraUniformData.hpp
        Stardust/Resources/Geometry.hpp
        Stardust/Resources/VertexData.hpp
        Stardust/Resources/Primitives/Cube.hpp
        Stardust/Resources/Primitives/Sphere.hpp

        Stardust/Vulkan/Barrier.hpp Stardust/Vulkan/Barrier.cpp
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

        Stardust/Vulkan/Presentation/Swapchain.cpp Stardust/Vulkan/Presentation/Swapchain.hpp
        Stardust/Vulkan/Presentation/SwapchainBuilder.hpp Stardust/Vulkan/Presentation/SwapchainBuilder.cpp
        Stardust/Vulkan/Presentation/SwapchainCapabilities.hpp

        Stardust/Vulkan/Image/DepthBuffer.cpp Stardust/Vulkan/Image/DepthBuffer.hpp
        Stardust/Vulkan/Image/Texture.cpp Stardust/Vulkan/Image/Texture.hpp

        Stardust/Vulkan/Raytracing/Blas.cpp Stardust/Vulkan/Raytracing/Blas.hpp
        Stardust/Vulkan/Raytracing/Tlas.cpp Stardust/Vulkan/Raytracing/Tlas.hpp
        Stardust/Vulkan/Raytracing/ShaderBindingTable.hpp

        Stardust/Vulkan/v2/Descriptor.tpp Stardust/Vulkan/v2/Descriptor.hpp

        Stardust/Scene/Transform.hpp
        Stardust/Scene/Object.hpp
        Stardust/Scene/Light.hpp
        Stardust/Scene/Camera.cpp Stardust/Scene/Camera.hpp

        Stardust/Rendering/RenderSettings.hpp
        Stardust/Rendering/AmbientOcclusion.hpp Stardust/Rendering/AmbientOcclusion.cpp
        Stardust/Rendering/RTAO/RTAOKernel.hpp Stardust/Rendering/RTAO/RTAOKernel.cpp
        Stardust/Rendering/RTAO/RTAOParams.hpp

        Stardust/RenderGraph/RenderGraph.hpp
        Stardust/RenderGraph/Node.hpp
        Stardust/RenderGraph/Input.hpp
        Stardust/RenderGraph/Output.hpp
        Stardust/RenderGraph/Scene.hpp

        Stardust/RenderGraph/graph/RenderPath.hpp

        Stardust/RenderGraph/node/RTAONode.hpp Stardust/RenderGraph/node/RTAONode.cpp
        Stardust/RenderGraph/node/OffscreenRenderNode.cpp Stardust/RenderGraph/node/OffscreenRenderNode.hpp
        Stardust/RenderGraph/node/SceneNode.cpp Stardust/RenderGraph/node/SceneNode.hpp
        Stardust/RenderGraph/node/CompositionNode.cpp Stardust/RenderGraph/node/CompositionNode.hpp
        Stardust/RenderGraph/node/RayTracingNode.cpp Stardust/RenderGraph/node/RayTracingNode.hpp

        Stardust/RenderGraph/res/AccelerationStructureResource.hpp
        Stardust/RenderGraph/res/CameraResource.hpp
        Stardust/RenderGraph/res/ImageResource.hpp
        Stardust/RenderGraph/res/pcObject.hpp
        Stardust/RenderGraph/res/ObjectsResource.hpp

        Stardust/RenderGraph/editor/RenderGraphEditor.cpp Stardust/RenderGraph/editor/RenderGraphEditor.hpp
        Stardust/RenderGraph/graph/Graph.cpp Stardust/RenderGraph/graph/Graph.hpp Stardust/Utility.hpp)

target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm nlohmann_json::nlohmann_json)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        Stardust
        ThirdParty/stb
        ThirdParty/tinyobj
        ThirdParty/stduuid)
target_compile_definitions(${PROJECT_NAME} PUBLIC
        IMGUI_DEFINE_MATH_OPERATORS
        UUID_SYSTEM_GENERATOR
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        SD_DEBUG
        -DImTextureID=ImU64)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${PYTHON} compile_shaders.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})