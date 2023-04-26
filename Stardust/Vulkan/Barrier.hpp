#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Image/Image.hpp>

namespace sdvk
{
    class ImageMemoryBarrier
    {
    private:
        ImageMemoryBarrier(vk::AccessFlagBits src, vk::AccessFlagBits dst,
                           vk::ImageLayout current, vk::ImageLayout target,
                           vk::ImageSubresourceRange image_subresource_range);

    public:
        struct Builder
        {
            Builder& access_mask(vk::AccessFlagBits src, vk::AccessFlagBits dst);
            Builder& layout(vk::ImageLayout current, vk::ImageLayout target);
            Builder& with_subresource_range(vk::ImageSubresourceRange image_subresource_range);
            Builder& with_image(const std::shared_ptr<sdvk::Image>& image);

            ImageMemoryBarrier create();

        private:
            vk::AccessFlagBits           _src, _dst;
            vk::ImageLayout              _current, _target;
            std::shared_ptr<sdvk::Image> _image;
            vk::ImageSubresourceRange    _isrr = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
        };

        void set_image(const vk::Image& image);

        void insert(vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage, const vk::CommandBuffer& command_buffer);

        void undo(const vk::CommandBuffer& command_buffer);

        void wrap(vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage, const vk::CommandBuffer& command_buffer, const std::function<void()>& fn);

    private:
        // Cache for remove
        vk::PipelineStageFlagBits last_src_stage {};
        vk::PipelineStageFlagBits last_dst_stage {};

        vk::ImageMemoryBarrier m_barrier {};
    };
}