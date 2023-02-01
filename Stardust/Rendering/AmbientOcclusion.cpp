#include "AmbientOcclusion.hpp"

namespace sd
{

    AmbientOcclusion::AmbientOcclusion(const sdvk::CommandBuffers& command_buffers, const sdvk::Context& context)
    : m_command_buffers(command_buffers), m_context(context) {}
}