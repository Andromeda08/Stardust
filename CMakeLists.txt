cmake_minimum_required(VERSION 3.23)
project(Stardust)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(PYTHON "py")

# Set up dependencies
find_package(Vulkan REQUIRED)

set(BUILD_SHARED_LIBS ON FORCE)
add_subdirectory(ThirdParty/glfw)

add_subdirectory(ThirdParty/stduuid)

add_subdirectory(ThirdParty/glm)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(IMGUI_DIR ThirdParty/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends)

set(IMNODES_DIR ThirdParty/imnodes)
include_directories(${IMNODES_DIR})

add_executable(${PROJECT_NAME}
        Stardust/main.cpp
        # Stardust/pch.hpp

        ThirdParty/stb/stb_image.cpp ThirdParty/stb/stb_image.h
        ThirdParty/tinyobj/tiny_obj_impl.cpp

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
        Stardust/Utility.hpp

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

        Stardust/Vulkan/Image/Sampler.hpp

        Stardust/Vulkan/Rendering/ShaderModule.cpp Stardust/Vulkan/Rendering/ShaderModule.hpp
        Stardust/Vulkan/Rendering/Mesh.cpp Stardust/Vulkan/Rendering/Mesh.hpp
        Stardust/Vulkan/Rendering/Pipeline.hpp
        Stardust/Vulkan/Rendering/PipelineBuilder.cpp Stardust/Vulkan/Rendering/PipelineBuilder.hpp
        Stardust/Vulkan/Rendering/PipelineState.cpp Stardust/Vulkan/Rendering/PipelineState.hpp
        Stardust/Vulkan/Rendering/RenderPass.hpp Stardust/Vulkan/Rendering/RenderPass.cpp

        Stardust/Vulkan/Presentation/Swapchain.cpp Stardust/Vulkan/Presentation/Swapchain.hpp
        Stardust/Vulkan/Presentation/SwapchainBuilder.hpp Stardust/Vulkan/Presentation/SwapchainBuilder.cpp
        Stardust/Vulkan/Presentation/SwapchainCapabilities.hpp

        Stardust/Vulkan/Raytracing/Blas.cpp Stardust/Vulkan/Raytracing/Blas.hpp
        Stardust/Vulkan/Raytracing/Tlas.cpp Stardust/Vulkan/Raytracing/Tlas.hpp
        Stardust/Vulkan/Raytracing/ShaderBindingTable.hpp

        Stardust/Scene/Transform.hpp
        Stardust/Scene/Object.hpp
        Stardust/Scene/Light.hpp
        Stardust/Scene/Camera.cpp Stardust/Scene/Camera.hpp
        Stardust/Scene/Scene.hpp Stardust/Scene/Scene.cpp

        Stardust/Nebula/Barrier.hpp Stardust/Nebula/Barrier.cpp
        Stardust/Nebula/Descriptor.hpp Stardust/Nebula/Descriptor.cpp
        Stardust/Nebula/Framebuffer.hpp Stardust/Nebula/Framebuffer.cpp
        Stardust/Nebula/Image.hpp Stardust/Nebula/Image.cpp
        Stardust/Nebula/ImageResolve.hpp
        Stardust/Nebula/ImageBlit.hpp

        Stardust/Math/Graph/Vertex.hpp

        Stardust/VirtualGraph/Compile/GraphCompileStrategy.hpp Stardust/VirtualGraph/Compile/GraphCompileStrategy.cpp
        Stardust/VirtualGraph/Common/GraphContext.hpp Stardust/VirtualGraph/Common/GraphContext.cpp
        Stardust/VirtualGraph/Common/NodeType.hpp
        Stardust/VirtualGraph/Common/RenderPath.hpp
        Stardust/VirtualGraph/Compile/DefaultCompileStrategy.hpp Stardust/VirtualGraph/Compile/DefaultCompileStrategy.cpp
        Stardust/VirtualGraph/Compile/NodeFactory.hpp Stardust/VirtualGraph/Compile/NodeFactory.cpp
        Stardust/VirtualGraph/Compile/CompileResult.hpp

        Stardust/VirtualGraph/Compile/Algorithm/Bfs.hpp Stardust/VirtualGraph/Compile/Algorithm/Bfs.cpp
        Stardust/VirtualGraph/Compile/Algorithm/TopologicalSort.hpp Stardust/VirtualGraph/Compile/Algorithm/TopologicalSort.cpp

        Stardust/VirtualGraph/Editor/Edge.hpp
        Stardust/VirtualGraph/Editor/GraphEditor.hpp Stardust/VirtualGraph/Editor/GraphEditor.cpp
        Stardust/VirtualGraph/Editor/Node.hpp Stardust/VirtualGraph/Editor/Node.cpp
        Stardust/VirtualGraph/Editor/ResourceDescription.hpp

        Stardust/VirtualGraph/RenderGraph/Nodes/Node.hpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.hpp
        Stardust/VirtualGraph/RenderGraph/Nodes/BlurNode.hpp Stardust/VirtualGraph/RenderGraph/Nodes/BlurNode.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.hpp Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusionNode.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AntiAliasingNode.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/DeferredRender.hpp Stardust/VirtualGraph/RenderGraph/Nodes/DeferredRender.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/LightingPass.hpp Stardust/VirtualGraph/RenderGraph/Nodes/LightingPass.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/PresentNode.hpp Stardust/VirtualGraph/RenderGraph/Nodes/PresentNode.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/SceneProviderNode.hpp Stardust/VirtualGraph/RenderGraph/Nodes/SceneProviderNode.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionMode.hpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionOptions.hpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionStrategy.hpp Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/AmbientOcclusionStrategy.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/ScreenSpaceAO.hpp Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/ScreenSpaceAO.cpp
        Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/RayTracedAO.hpp Stardust/VirtualGraph/RenderGraph/Nodes/AmbientOcclusion/RayTracedAO.cpp

        Stardust/VirtualGraph/RenderGraph/Resources/Resource.hpp
        Stardust/VirtualGraph/RenderGraph/Resources/ResourceSpecification.hpp
        Stardust/VirtualGraph/RenderGraph/Resources/ResourceType.hpp Stardust/VirtualGraph/RenderGraph/Resources/ResourceType.cpp
        Stardust/VirtualGraph/RenderGraph/Resources/ResourceRole.hpp Stardust/VirtualGraph/RenderGraph/Resources/ResourceRole.cpp
        Stardust/VirtualGraph/Builder/Builder.h Stardust/VirtualGraph/Builder/Builder.cpp
)

# target_precompile_headers(${PROJECT_NAME} PRIVATE Stardust/pch.hpp)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw glm stduuid)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        Stardust
        ThirdParty/stb
        ThirdParty/tinyobj
        ThirdParty/stduuid/include
        ThirdParty/glfw/include)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        IMGUI_DEFINE_MATH_OPERATORS
        UUID_SYSTEM_GENERATOR
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        SD_DEBUG
        -DImTextureID=ImU64)
