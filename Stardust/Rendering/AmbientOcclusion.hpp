#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <Vulkan/CommandBuffers.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Image/Image.hpp>

namespace sd
{
    enum class AmbientOcclusionMode
    {
        eNone,
        eRTAO,
        eSSAO
    };

    class AmbientOcclusion
    {
    public:
        AmbientOcclusion(sdvk::CommandBuffers const& command_buffers, sdvk::Context const& context);

        virtual ~AmbientOcclusion() = default;

        virtual void run(const glm::mat4& view_mtx, const vk::CommandBuffer& command_buffer) = 0;

        const sdvk::Image& get_result() const { return *m_ao_buffer; }

    protected:
        std::unique_ptr<sdvk::Image> m_ao_buffer;

        const sdvk::CommandBuffers& m_command_buffers;
        const sdvk::Context& m_context;
    };
}
