#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace sdvk
{
    class Context;
}

namespace Nebula
{
    class Framebuffer
    {
    public:
        struct Builder
        {
        public:
            Builder& add_attachment(const vk::ImageView& image_view);

            Builder& set_render_pass(const vk::RenderPass& render_pass);

            Builder& set_size(vk::Extent2D size);

            Builder& set_count(uint32_t count);

            std::shared_ptr<Framebuffer> create(const sdvk::Context& ctx);

        private:
            uint32_t _count {2};
            std::vector<vk::ImageView> _attachments;
            vk::RenderPass _render_pass;
            vk::Extent2D _size;
        };

        Framebuffer(const std::vector<vk::ImageView>& attachments,
                    const vk::RenderPass& render_pass,
                    const vk::Extent2D& size,
                    uint32_t count,
                    const sdvk::Context& ctx);

        const vk::Framebuffer& get(uint32_t index);

        const vk::Framebuffer& operator[](uint32_t index);

    private:
        std::vector<vk::Framebuffer> m_framebuffers;
    };
}