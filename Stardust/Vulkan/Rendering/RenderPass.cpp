#include "RenderPass.hpp"

namespace sdvk
{
    RenderPass::Builder& RenderPass::Builder::add_color_attachment(vk::Format format, vk::SampleCountFlagBits sample_count, vk::ImageLayout final_layout, vk::AttachmentLoadOp load_op)
    {
        vk::AttachmentDescription ad;
        ad.setFormat(format);
        ad.setSamples(sample_count);
        ad.setLoadOp(load_op);
        ad.setStoreOp(vk::AttachmentStoreOp::eStore);
        ad.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        ad.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        ad.setInitialLayout(vk::ImageLayout::eUndefined);
        ad.setFinalLayout(final_layout);
        _attachments.push_back(ad);

        vk::AttachmentReference ref = {};
        ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        ref.setAttachment(_attachments.size() - 1);
        _color_refs.push_back(ref);

        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::set_depth_attachment(vk::Format format, vk::SampleCountFlagBits sample_count)
    {
        vk::AttachmentDescription ad;
        ad.setFormat(format);
        ad.setSamples(sample_count);
        ad.setLoadOp(vk::AttachmentLoadOp::eClear);
        ad.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        ad.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        ad.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        ad.setInitialLayout(vk::ImageLayout::eUndefined);
        ad.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        _attachments.push_back(ad);

        _has_depth_attachment = true;
        _depth_ref.setAttachment(_attachments.size() - 1);
        _depth_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::set_resolve_attachment(vk::Format format, vk::ImageLayout final_layout)
    {
        vk::AttachmentDescription ad;
        ad.setFormat(format);
        ad.setSamples(vk::SampleCountFlagBits::e1);
        ad.setLoadOp(vk::AttachmentLoadOp::eDontCare);
        ad.setStoreOp(vk::AttachmentStoreOp::eStore);
        ad.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        ad.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        ad.setInitialLayout(vk::ImageLayout::eUndefined);
        ad.setFinalLayout(final_layout);
        _attachments.push_back(ad);

        _has_resolve_attachment = true;
        _resolve_ref.setAttachment(_attachments.size() - 1);
        _resolve_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::make_subpass(vk::PipelineBindPoint pipeline_bind_point)
    {
        vk::SubpassDescription subpass = {};
        subpass.setColorAttachmentCount(static_cast<uint32_t>(_color_refs.size()));
        subpass.setInputAttachmentCount(0);
        subpass.setPInputAttachments(nullptr);
        subpass.setPResolveAttachments(_has_resolve_attachment ? &_resolve_ref : nullptr);
        subpass.setPColorAttachments(_color_refs.data());
        subpass.setPDepthStencilAttachment(_has_depth_attachment ? &_depth_ref : nullptr);
        subpass.setPipelineBindPoint(pipeline_bind_point);
        _subpass = subpass;

        _subpass_dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
        _subpass_dependency.setDstSubpass(0);
        _subpass_dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
        _subpass_dependency.setSrcAccessMask({});
        _subpass_dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
        _subpass_dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::with_name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    vk::RenderPass RenderPass::Builder::create(const Context& context)
    {
        vk::RenderPassCreateInfo create_info;
        create_info.setAttachmentCount(_attachments.size());
        create_info.setPAttachments(_attachments.data());
        create_info.setSubpassCount(1);
        create_info.setPSubpasses(&_subpass);
        create_info.setDependencyCount(1);
        create_info.setPDependencies(&_subpass_dependency);

        auto result = context.device().createRenderPass(&create_info, nullptr, &_render_pass);

        if (!_name.empty())
        {
            sdvk::util::name_vk_object(_name, (uint64_t) static_cast<VkRenderPass>(_render_pass), vk::ObjectType::eRenderPass, context.device());
        }

        return _render_pass;
    }

    RenderPass::Execute& RenderPass::Execute::with_render_pass(const vk::RenderPass& render_pass)
    {
        _begin_info.setRenderPass(render_pass);
        return *this;
    }

    RenderPass::Execute& RenderPass::Execute::with_render_area(vk::Rect2D render_area)
    {
        _begin_info.setRenderArea(render_area);
        return *this;
    }

    RenderPass::Execute& RenderPass::Execute::with_clear_values(const std::array<vk::ClearValue, 2>& clear_values)
    {
        _begin_info.setClearValueCount(clear_values.size());
        _begin_info.setPClearValues(clear_values.data());
        return *this;
    }

    RenderPass::Execute& RenderPass::Execute::with_clear_values(const std::array<vk::ClearValue, 3>& clear_values)
    {
        _begin_info.setClearValueCount(clear_values.size());
        _begin_info.setPClearValues(clear_values.data());
        return *this;
    }

    RenderPass::Execute& RenderPass::Execute::with_framebuffer(const vk::Framebuffer& framebuffer)
    {
        _begin_info.setFramebuffer(framebuffer);
        return *this;
    }

    void
    RenderPass::Execute::execute(const vk::CommandBuffer& cmd, const std::function<void(const vk::CommandBuffer&)>& fn)
    {
        cmd.beginRenderPass(&_begin_info, vk::SubpassContents::eInline);
        fn(cmd);
        cmd.endRenderPass();
    }

    RenderPass::Execute& RenderPass::Execute::with_clear_value(const std::array<vk::ClearValue, 1>& clear_value)
    {
        _begin_info.setClearValueCount(clear_value.size());
        _begin_info.setPClearValues(clear_value.data());
        return *this;
    }
}