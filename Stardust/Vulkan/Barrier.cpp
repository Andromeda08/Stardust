#include "Barrier.hpp"

namespace sdvk
{

    #pragma region Builder
    ImageMemoryBarrier::Builder&
    ImageMemoryBarrier::Builder::access_mask(vk::AccessFlagBits src, vk::AccessFlagBits dst)
    {
        _src = src;
        _dst = dst;
        return *this;
    }

    ImageMemoryBarrier::Builder&
    ImageMemoryBarrier::Builder::layout(vk::ImageLayout current, vk::ImageLayout target)
    {
        _current = current;
        _target  = target;
        return *this;
    }

    ImageMemoryBarrier::Builder&
    ImageMemoryBarrier::Builder::with_subresource_range(vk::ImageSubresourceRange image_subresource_range)
    {
        _isrr = image_subresource_range;
        return *this;
    }

    ImageMemoryBarrier::Builder& ImageMemoryBarrier::Builder::with_image(const std::shared_ptr<sdvk::Image>& image)
    {
        _image = image;
        return *this;
    }

    ImageMemoryBarrier ImageMemoryBarrier::Builder::create()
    {
        auto result = ImageMemoryBarrier(_src, _dst, _current, _target, _isrr);

        if (_image)
        {
            result.set_image(_image->image());
        }

        return result;
    }
    #pragma endregion

    ImageMemoryBarrier::ImageMemoryBarrier(vk::AccessFlagBits src, vk::AccessFlagBits dst,
                                           vk::ImageLayout current, vk::ImageLayout target,
                                           vk::ImageSubresourceRange image_subresource_range)
    {
        m_barrier.setSrcAccessMask(src);
        m_barrier.setDstAccessMask(dst);
        m_barrier.setOldLayout(current);
        m_barrier.setNewLayout(target);
        m_barrier.setSubresourceRange(image_subresource_range);

        m_inverse.setSrcAccessMask(dst);
        m_inverse.setDstAccessMask(src);
        m_inverse.setOldLayout(target);
        m_inverse.setNewLayout(current);
        m_inverse.setSubresourceRange(image_subresource_range);
    }

    void ImageMemoryBarrier::set_image(const vk::Image& image)
    {
        m_barrier.setImage(image);
        m_inverse.setImage(image);
    }

    void ImageMemoryBarrier::insert(vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage,
                                    const vk::CommandBuffer& command_buffer)
    {
        command_buffer.pipelineBarrier(src_stage, dst_stage,
                                       {},
                                       0, nullptr,
                                       0,nullptr,
                                       1,&m_barrier);

    }

    void ImageMemoryBarrier::undo(vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage, const vk::CommandBuffer& command_buffer)
    {
        command_buffer.pipelineBarrier(src_stage, dst_stage,
                                       {},
                                       0, nullptr,
                                       0,nullptr,
                                       1,&m_inverse);

/*        vk::AccessFlags src { m_barrier.dstAccessMask }, dst { m_barrier.srcAccessMask };
        vk::ImageLayout from { m_barrier.newLayout }, to { m_barrier.oldLayout };

        m_barrier.setOldLayout(from);
        m_barrier.setNewLayout(to);
        m_barrier.setSrcAccessMask(src);
        m_barrier.setDstAccessMask(dst);

        command_buffer.pipelineBarrier(last_dst_stage, last_src_stage,
                                       {},
                                       0, nullptr,
                                       0,nullptr,
                                       1,&m_barrier);

        m_barrier.setOldLayout(to);
        m_barrier.setNewLayout(from);
        m_barrier.setSrcAccessMask(dst);
        m_barrier.setDstAccessMask(src);*/
    }

    void ImageMemoryBarrier::wrap(vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage,
                                  const vk::CommandBuffer& command_buffer, const std::function<void()>& fn)
    {
        insert(src_stage, dst_stage, command_buffer);
        fn();
        undo(dst_stage, src_stage, command_buffer);
    }
}