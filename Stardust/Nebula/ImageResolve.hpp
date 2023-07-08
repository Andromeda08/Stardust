#pragma once

#include <array>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <Nebula/Barrier.hpp>
#include <Nebula/Image.hpp>

namespace Nebula::Sync
{
    class ImageResolve
    {
    public:
        ImageResolve(const std::shared_ptr<Image>& src_image, const std::shared_ptr<Image>& dst_image)
        : m_src_image(src_image), m_dst_image(dst_image)
        {
            auto& src_props = src_image->properties();
            auto& src_state = src_image->state();

            auto& dst_props = dst_image->properties();
            auto& dst_state = dst_image->state();

            if (src_props.extent != dst_props.extent)
            {
                // TODO: error I guess
            }

            m_barriers[0] = std::make_unique<ImageBarrier>(src_image,
                                                           src_state.layout, vk::ImageLayout::eTransferSrcOptimal,
                                                           src_state.access_flags, vk::AccessFlagBits2::eTransferRead,
                                                           vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eAllTransfer);

            m_barriers[1] = std::make_unique<ImageBarrier>(dst_image,
                                                           dst_state.layout, vk::ImageLayout::eTransferDstOptimal,
                                                           dst_state.access_flags, vk::AccessFlagBits2::eTransferWrite,
                                                           vk::PipelineStageFlagBits2::eNone, vk::PipelineStageFlagBits2::eAllTransfer);

            m_image_resolve.setSrcOffset({ 0, 0 });
            m_image_resolve.setDstOffset({ 0, 0 });
            m_image_resolve.setSrcSubresource(src_props.subresource_layers);
            m_image_resolve.setDstSubresource(dst_props.subresource_layers);
            m_image_resolve.setExtent({ src_props.extent.width, src_props.extent.height, 1 });

            m_resolve_image_info.setRegionCount(1);
            m_resolve_image_info.setPRegions(&m_image_resolve);
            m_resolve_image_info.setSrcImageLayout(vk::ImageLayout::eTransferSrcOptimal);
            m_resolve_image_info.setDstImageLayout(vk::ImageLayout::eTransferDstOptimal);
        }

        void resolve(const vk::CommandBuffer& command_buffer)
        {
            auto src_image = m_src_image.lock();
            auto dst_image = m_dst_image.lock();

            if (src_image == nullptr || dst_image == nullptr)
            {
                // TODO: error I guess
            }

            m_resolve_image_info.setSrcImage(src_image->image());
            m_resolve_image_info.setDstImage(dst_image->image());

            // 1. SRC_Current_Layout -> TransferSRC | DST_Current_Layout -> TransferDST
            for (const auto& b : m_barriers) b->apply(command_buffer);

            // 2. Resolve Image
            command_buffer.resolveImage2(&m_resolve_image_info);

            // 3. TransferSRC -> SRC_Old_Layout | TransferDST -> DST_Old_Layout
            for (const auto& b : m_barriers) b->revert(command_buffer);
        }

    private:
        vk::ResolveImageInfo2 m_resolve_image_info {};
        vk::ImageResolve2 m_image_resolve {};

        std::array<std::unique_ptr<ImageBarrier>, 2> m_barriers;

        std::weak_ptr<Image> m_src_image;
        std::weak_ptr<Image> m_dst_image;
    };
}