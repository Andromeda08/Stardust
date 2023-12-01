#include "Barrier.hpp"
#include <Nebula/Image.hpp>

namespace Nebula::Sync
{
    void Barrier::wrap(const vk::CommandBuffer& command_buffer, const std::function<void()>& lambda)
    {
        apply(command_buffer);
        lambda();
        revert(command_buffer);
    }

    ImageBarrier::ImageBarrier(const std::shared_ptr<Image>& image,
                               vk::ImageLayout old_layout, vk::ImageLayout new_layout,
                               vk::AccessFlags2 src_access_flags, vk::AccessFlags2 dst_access_flags,
                               vk::PipelineStageFlags2 src_stage, vk::PipelineStageFlags2 dst_stage)
    : Barrier()
    , m_image(image)
    {
        auto& props = image->properties();

        m_barrier.setOldLayout(old_layout);
        m_barrier.setNewLayout(new_layout);
        m_barrier.setSrcAccessMask(src_access_flags);
        m_barrier.setDstAccessMask(dst_access_flags);
        m_barrier.setSrcStageMask(src_stage);
        m_barrier.setDstStageMask(dst_stage);
        m_barrier.setSubresourceRange(props.subresource_range);
        m_barrier.setImage(image->image());

        m_anti_barrier.setOldLayout(new_layout);
        m_anti_barrier.setNewLayout(old_layout);
        m_anti_barrier.setSrcAccessMask(dst_access_flags);
        m_anti_barrier.setDstAccessMask(src_access_flags);
        m_anti_barrier.setSrcStageMask(dst_stage);
        m_anti_barrier.setDstStageMask(src_stage);
        m_anti_barrier.setSubresourceRange(props.subresource_range);
        m_anti_barrier.setImage(image->image());

        m_dependency_info.setImageMemoryBarrierCount(1);
    }

    void ImageBarrier::apply(const vk::CommandBuffer& command_buffer)
    {
        auto image = m_image.lock();
        if (image == nullptr)
        {
            throw std::runtime_error("Image no longer exists.");
        }

        image->update_state({ m_barrier.dstAccessMask, m_barrier.newLayout });

        m_dependency_info.setPImageMemoryBarriers(&m_barrier);
        command_buffer.pipelineBarrier2(&m_dependency_info);
    }

    void ImageBarrier::revert(const vk::CommandBuffer& command_buffer)
    {
        auto image = m_image.lock();
        if (image == nullptr)
        {
            throw std::runtime_error("Image no longer exists.");
        }

        image->update_state({ m_barrier.srcAccessMask, m_barrier.oldLayout });

        m_dependency_info.setPImageMemoryBarriers(&m_anti_barrier);
        command_buffer.pipelineBarrier2(&m_dependency_info);
    }

    void ImageBarrierBatch::apply(const vk::CommandBuffer& command_buffer)
    {
        std::vector<vk::ImageMemoryBarrier2> barriers;
        for (auto& barrier : m_barriers)
        {
            auto image = barrier.m_image.lock();
            if (image == nullptr)
            {
                throw std::runtime_error("Image no longer exists.");
            }
            image->update_state({ barrier.m_barrier.dstAccessMask, barrier.m_barrier.newLayout });
            barriers.push_back(barrier.m_barrier);
        }

        m_dependency_info.setImageMemoryBarrierCount(m_barriers.size());
        m_dependency_info.setPImageMemoryBarriers(barriers.data());

        command_buffer.pipelineBarrier2(&m_dependency_info);
    }
}
