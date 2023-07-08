#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace Nebula
{
    class Image;
}

namespace Nebula::Sync
{
    class Barrier
    {
    public:
        //Insert the specified barrier. (old -> new)
        virtual void apply(const vk::CommandBuffer& command_buffer) = 0;

        // Apply the inverse of the specified barrier. (new -> old)
        virtual void revert(const vk::CommandBuffer& command_buffer) = 0;

        // Executes the given lambda after inserting a barrier then immediately executes the inverse of the barrier.
        void wrap(const vk::CommandBuffer& command_buffer, const std::function<void()>& lambda);

        Barrier() = default;
        ~Barrier() = default;
    };

    class ImageBarrier : public Barrier
    {
    public:
        ImageBarrier(const std::shared_ptr<Image>& image,
                     vk::ImageLayout old_layout,
                     vk::ImageLayout new_layout,
                     vk::AccessFlags2 src_access_flags = vk::AccessFlagBits2::eNone,
                     vk::AccessFlags2 dst_access_flags = vk::AccessFlagBits2::eNone,
                     vk::PipelineStageFlags2 src_stage = vk::PipelineStageFlagBits2::eNone,
                     vk::PipelineStageFlags2 dst_stage = vk::PipelineStageFlagBits2::eNone);

        ~ImageBarrier() = default;

        void apply(const vk::CommandBuffer& command_buffer) override;

        void revert(const vk::CommandBuffer& command_buffer) override;

    private:
        vk::DependencyInfo m_dependency_info {};
        vk::ImageMemoryBarrier2 m_barrier {};
        vk::ImageMemoryBarrier2 m_anti_barrier {};

        std::weak_ptr<Image> m_image;
    };
}