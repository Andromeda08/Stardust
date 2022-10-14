#pragma once

#include <memory>
#include "Buffer.hpp"
#include "../Device.hpp"
#include "../Command/CommandBuffers.hpp"
#include "../../Macro.hpp"
#include "../../Resources/UniformBufferObject.hpp"

class UniformBuffer
{
public:
    NON_COPIABLE(UniformBuffer)

    explicit UniformBuffer(const Device& device);

    UniformBuffer(const UniformBufferObject& ubo, const Device& device);

    void update(const UniformBufferObject& ubo) const;

    vk::Buffer handle() const { return m_buffer->handle(); }

    const Buffer& buffer() const { return *m_buffer; }

    const Device& device() const { return m_device; }

private:
    std::unique_ptr<Buffer> m_buffer;

    const Device& m_device;
};
