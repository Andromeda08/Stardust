#pragma once

#include <array>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>

namespace Nebula::Sync
{
    class ImageBlit
    {
    public:
        ImageBlit(const std::shared_ptr<Image>& src_image, const std::shared_ptr<Image>& dst_image)
        : m_src_image(src_image), m_dst_image(dst_image)
        {
            auto& src_state = src_image->state();
            auto& dst_state = dst_image->state();

            m_barriers[0] = std::make_unique<ImageBarrier>(src_image,
                                                           src_state.layout, vk::ImageLayout::eTransferSrcOptimal,
                                                           src_state.access_flags, vk::AccessFlagBits2::eTransferRead,
                                                           vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eAllTransfer);

            m_barriers[1] = std::make_unique<ImageBarrier>(dst_image,
                                                           dst_state.layout, vk::ImageLayout::eTransferDstOptimal,
                                                           dst_state.access_flags, vk::AccessFlagBits2::eTransferWrite,
                                                           vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eAllTransfer);

            m_image_blit.setSrcOffsets({0, 0});
            m_image_blit.setDstOffsets({0, 0});
            m_image_blit.setSrcSubresource(src_image->properties().subresource_layers);
            m_image_blit.setDstSubresource(dst_image->properties().subresource_layers);

            m_blit_image_info.setFilter(vk::Filter::eNearest);
            m_blit_image_info.setRegionCount(1);
            m_blit_image_info.setPRegions(&m_image_blit);
            m_blit_image_info.setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal);
            m_blit_image_info.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);
        }

        void blit(const vk::CommandBuffer& command_buffer)
        {
            auto src_image = m_src_image.lock();
            auto dst_image = m_dst_image.lock();

            m_blit_image_info.setSrcImage(src_image->image());
            m_blit_image_info.setDstImage(dst_image->image());

            for (const auto& b : m_barriers) b->apply(command_buffer);
            command_buffer.blitImage2(&m_blit_image_info);
            for (const auto& b : m_barriers) b->revert(command_buffer);
        }

    private:
        vk::BlitImageInfo2 m_blit_image_info {};
        vk::ImageBlit2 m_image_blit {};

        std::array<std::unique_ptr<ImageBarrier>, 2> m_barriers;

        std::weak_ptr<Image> m_src_image;
        std::weak_ptr<Image> m_dst_image;
    };
}