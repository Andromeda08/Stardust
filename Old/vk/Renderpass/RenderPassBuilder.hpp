#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk/Image.hpp>
#include <vk/Device/Device.hpp>

struct RenderPass2
{
    vk::RenderPass  render_pass { VK_NULL_HANDLE };
    vk::Framebuffer framebuffer { VK_NULL_HANDLE };
    std::vector<vk::ImageView> fb_attachments = {};
};

struct RenderPassBuilder
{
    using T = RenderPassBuilder;

    explicit RenderPassBuilder(Device const& d): _device(d) {}

    T& add_color_attachment(vk::Format format, vk::ImageLayout final_layout)
    {
        vk::AttachmentDescription desc;
        desc.setFinalLayout(final_layout);
        desc.setFormat(format);
        desc.setSamples(vk::SampleCountFlagBits::e1);
        desc.setLoadOp(vk::AttachmentLoadOp::eClear);
        desc.setStoreOp(vk::AttachmentStoreOp::eStore);
        desc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        desc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        desc.setInitialLayout(vk::ImageLayout::eUndefined);

        _attachment_descriptions.push_back(desc);

        vk::AttachmentReference ref;
        ref.setAttachment(_attachment_descriptions.size() - 1);
        ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        _color_references.push_back(ref);

        return *this;
    }

    T& set_depth_attachment(vk::Format format, vk::ImageLayout final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal)
    {
        vk::AttachmentDescription desc;
        desc.setFinalLayout(final_layout);
        desc.setFormat(format);
        desc.setSamples(vk::SampleCountFlagBits::e1);
        desc.setLoadOp(vk::AttachmentLoadOp::eClear);
        desc.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        desc.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
        desc.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
        desc.setInitialLayout(vk::ImageLayout::eUndefined);

        _attachment_descriptions.push_back(desc);

        vk::AttachmentReference ref;
        ref.setAttachment(_attachment_descriptions.size() - 1);
        ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        _depth_reference = ref;
        _depth_attachment = true;

        return *this;
    }

    T& make_subpass_description(vk::PipelineBindPoint bind_point)
    {
        _subpass_description.setPipelineBindPoint(bind_point);
        _subpass_description.setColorAttachmentCount(static_cast<uint32_t>(_color_references.size()));
        _subpass_description.setPColorAttachments(_color_references.data());
        _subpass_description.setPDepthStencilAttachment((_depth_attachment) ? &_depth_reference : nullptr);
        return *this;
    }

    T& add_subpass_dependency(uint32_t src_subpass, uint32_t dst_subpass,
                              vk::PipelineStageFlags src_stage, vk::PipelineStageFlags dst_stage,
                              vk::AccessFlags src_access, vk::AccessFlags dst_access,
                              vk::DependencyFlags flags = {})
    {
        vk::SubpassDependency dep;
        dep.setSrcSubpass(src_subpass);
        dep.setSrcStageMask(src_stage);
        dep.setSrcAccessMask(src_access);
        dep.setDstSubpass(dst_subpass);
        dep.setDstStageMask(dst_stage);
        dep.setDstAccessMask(dst_access);
        dep.setDependencyFlags(flags);

        _subpass_dependencies.push_back(dep);

        return *this;
    }

    T& add_framebuffer_attachment(re::Image const& image)
    {
        _result.fb_attachments.push_back(image.view());
        return *this;
    }

    T& make_framebuffer(vk::Extent2D dimensions)
    {
        _fb_extent = dimensions;
        _build_framebuffer = true;
        return *this;
    }

    RenderPass2 create()
    {
        vk::Result result;

        vk::RenderPassCreateInfo create_info;
        create_info.setAttachmentCount(static_cast<uint32_t>(_attachment_descriptions.size()));
        create_info.setPAttachments(_attachment_descriptions.data());
        create_info.setSubpassCount(1);
        create_info.setPSubpasses(&_subpass_description);
        create_info.setDependencyCount(static_cast<uint32_t>(_subpass_dependencies.size()));
        create_info.setPDependencies(_subpass_dependencies.data());

        result = _device.handle().createRenderPass(&create_info, nullptr, &_result.render_pass);

        if (_build_framebuffer)
        {
            vk::FramebufferCreateInfo fb_create_info;
            fb_create_info.setRenderPass(_result.render_pass);
            fb_create_info.setAttachmentCount(static_cast<uint32_t>(_result.fb_attachments.size()));
            fb_create_info.setPAttachments(_result.fb_attachments.data());
            fb_create_info.setWidth(_fb_extent.width);
            fb_create_info.setHeight(_fb_extent.height);
            fb_create_info.setLayers(1);

            result = _device.handle().createFramebuffer(&fb_create_info, nullptr, &_result.framebuffer);
        }

        if (!_build_framebuffer)
        {
            _result.fb_attachments.clear();
            _result.framebuffer = nullptr;
        }

        return _result;
    }

private:
    std::vector<vk::AttachmentDescription> _attachment_descriptions;

    std::vector<vk::AttachmentReference> _color_references;

    bool _depth_attachment = false;
    vk::AttachmentReference              _depth_reference;

    // TODO: Multiple subpasses?
    vk::SubpassDescription             _subpass_description;
    std::vector<vk::SubpassDependency> _subpass_dependencies;

    bool _build_framebuffer = false;
    vk::Extent2D _fb_extent = { 0, 0 };

    RenderPass2 _result;

    Device const& _device;
};