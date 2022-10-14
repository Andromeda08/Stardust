#include "RenderPass.hpp"

RenderPass::RenderPass(const Device& device, vk::Format swap_chain_img_format, vk::Format depth_img_format)
: m_device(device)
{
    std::vector<vk::AttachmentDescription> attachments = {
        make_color_attachment(swap_chain_img_format),
        make_depth_attachment(depth_img_format)
    };
    vk::AttachmentReference color_ref;
    color_ref.setAttachment(0);
    color_ref.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depth_ref;
    depth_ref.setAttachment(1);
    depth_ref.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&color_ref);
    subpass.setPDepthStencilAttachment(&depth_ref);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setSrcAccessMask({});
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    vk::RenderPassCreateInfo create_info;
    create_info.setAttachmentCount(attachments.size());
    create_info.setPAttachments(attachments.data());
    create_info.setSubpassCount(1);
    create_info.setPSubpasses(&subpass);
    create_info.setDependencyCount(1);
    create_info.setPDependencies(&dependency);

    auto result = m_device.handle().createRenderPass(&create_info, nullptr, &m_render_pass);
}

vk::AttachmentDescription RenderPass::make_color_attachment(vk::Format swap_chain_img_format)
{
    vk::AttachmentDescription color_attachment;

    color_attachment.setFormat(swap_chain_img_format);
    color_attachment.setSamples(vk::SampleCountFlagBits::e1);
    color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    return color_attachment;
}

vk::AttachmentDescription RenderPass::make_depth_attachment(vk::Format depth_img_format)
{
    vk::AttachmentDescription depth_attachment;

    depth_attachment.setFormat(depth_img_format);
    depth_attachment.setSamples(vk::SampleCountFlagBits::e1);
    depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    depth_attachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    return depth_attachment;
}
