#include "Buffer.hpp"

Buffer::Buffer(vk::DeviceSize buffer_size,
               vk::BufferUsageFlags usage_flags,
               vk::MemoryPropertyFlags property_flags,
               const Device& device)
: m_device(device)
{
    vk::Result result;

    vk::BufferCreateInfo buffer_info;
    buffer_info.setSharingMode(vk::SharingMode::eExclusive);
    buffer_info.setSize(buffer_size);
    buffer_info.setUsage(usage_flags);

    result = m_device.handle().createBuffer(&buffer_info, nullptr, &m_buffer);

    vk::MemoryRequirements memory_requirements = m_device.handle().getBufferMemoryRequirements(m_buffer);
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.setAllocationSize(memory_requirements.size);
    alloc_info.setMemoryTypeIndex(m_device.findMemoryType(memory_requirements.memoryTypeBits, property_flags));

    result = m_device.handle().allocateMemory(&alloc_info, nullptr, &m_memory);

    m_device.handle().bindBufferMemory(m_buffer, m_memory, 0);
}

Buffer::~Buffer()
{
    if (m_buffer)
    {
        m_device.handle().destroyBuffer(m_buffer, nullptr);
        m_device.handle().freeMemory(m_memory, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }
}

void Buffer::copy_buffer(const CommandBuffers& cmd,
                         vk::Buffer src,
                         vk::Buffer dst,
                         vk::DeviceSize buffer_size)
{
    auto cmd_buffer = cmd.begin_single_time();

    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(0);
    copy_region.setDstOffset(0);
    copy_region.setSize(buffer_size);

    cmd_buffer.copyBuffer(src, dst, 1, &copy_region);

    cmd.end_single_time(cmd_buffer);
}

Buffer::Buffer(vk::DeviceSize buffer_size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags property_flags,
               const Device &device, vk::DispatchLoaderDynamic dispatch)
: m_device(device)
{
    vk::Result result;

    vk::BufferCreateInfo buffer_info;
    buffer_info.setSharingMode(vk::SharingMode::eExclusive);
    buffer_info.setSize(buffer_size);
    buffer_info.setUsage(usage_flags);

    result = m_device.handle().createBuffer(&buffer_info, nullptr, &m_buffer, dispatch);

    vk::MemoryRequirements memory_requirements = m_device.handle().getBufferMemoryRequirements(m_buffer, dispatch);
    vk::MemoryAllocateInfo alloc_info;
    alloc_info.setAllocationSize(memory_requirements.size);
    alloc_info.setMemoryTypeIndex(m_device.findMemoryType(memory_requirements.memoryTypeBits, property_flags));

    result = m_device.handle().allocateMemory(&alloc_info, nullptr, &m_memory, dispatch);

    m_device.handle().bindBufferMemory(m_buffer, m_memory, 0, dispatch);

    vk::DebugUtilsObjectNameInfoEXT s1;
    s1.setObjectHandle((uint64_t) static_cast<VkBuffer>(m_buffer));
    s1.setObjectType(vk::ObjectType::eBuffer);
    s1.setPObjectName("Buffer");
    device.handle().setDebugUtilsObjectNameEXT(&s1, dispatch);

}
