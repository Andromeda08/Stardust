#pragma once

#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Context.hpp>
#include <Vulkan/Utils.hpp>

namespace sdvk
{
    class RenderPass
    {
    public:
        struct Builder
        {
            Builder() = default;

            Builder& add_color_attachment(vk::Format format,
                                          vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1,
                                          vk::ImageLayout final_layout = vk::ImageLayout::eColorAttachmentOptimal,
                                          vk::AttachmentLoadOp load_op = vk::AttachmentLoadOp::eClear);

            Builder& set_depth_attachment(vk::Format format,
                                          vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

            Builder& set_resolve_attachment(vk::Format format,
                                            vk::ImageLayout final_layout = vk::ImageLayout::ePresentSrcKHR);

            Builder& make_subpass(vk::PipelineBindPoint pipeline_bind_point = vk::PipelineBindPoint::eGraphics);

            Builder& with_name(std::string const& name);

            vk::RenderPass create(Context const& context);

        private:
            std::vector<vk::AttachmentDescription> _attachments;
            std::vector<vk::AttachmentReference> _color_refs;

            vk::AttachmentReference _depth_ref;
            bool _has_depth_attachment {false};

            vk::AttachmentReference _resolve_ref;
            bool _has_resolve_attachment {false};

            vk::SubpassDescription _subpass = {};
            vk::SubpassDependency  _subpass_dependency = {};

            vk::RenderPass _render_pass;

            std::string _name;
        };

        struct Execute
        {
            Execute() = default;

            Execute& with_render_pass(const vk::RenderPass& render_pass);

            Execute& with_render_area(vk::Rect2D render_area);

            Execute& with_clear_value(const std::array<vk::ClearValue, 1>& clear_value);

            Execute& with_clear_values(const std::array<vk::ClearValue, 2>& clear_values);

            Execute& with_clear_values(const std::array<vk::ClearValue, 3>& clear_values);

            Execute& with_clear_values(const std::array<vk::ClearValue, 4>& clear_values);

            template <unsigned int N>
            Execute& with_clear_values(const std::array<vk::ClearValue, N>& clear_values);

            Execute& with_framebuffer(const vk::Framebuffer& framebuffer);

            void execute(vk::CommandBuffer const& cmd, const std::function<void(const vk::CommandBuffer&)>& fn);

        private:
            vk::RenderPassBeginInfo _begin_info;
        };
    };

    template<unsigned int N>
    RenderPass::Execute& RenderPass::Execute::with_clear_values(const std::array<vk::ClearValue, N>& clear_values)
    {
        _begin_info.setClearValueCount(clear_values.size());
        _begin_info.setPClearValues(clear_values.data());
        return *this;
    }
}