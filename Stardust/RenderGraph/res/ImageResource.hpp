#pragma once
#include <memory>
#include <set>
#include <vulkan/vulkan.hpp>
#include <Vulkan/Image/Image.hpp>
#include "IInput.hpp"
#include "IOutput.hpp"

namespace sd::rg
{
    class ImageResource : public IInput, public IOutput
    {
    public:
        struct ImageResourceBuilder
        {
            ImageResourceBuilder& expect_depth_image()
            {
                expected_formats = {
                        vk::Format::eD16Unorm,  vk::Format::eD16UnormS8Uint,  vk::Format::eD24UnormS8Uint,
                        vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eX8D24UnormPack32
                };
                return *this;
            }

            ImageResourceBuilder& expect_color_image()
            {
                expected_formats = {
                        vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32Sfloat, vk::Format::eR32Sfloat,
                        vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16Sfloat, vk::Format::eR16G16Sfloat, vk::Format::eR16Sfloat,
                        vk::Format::eR8G8B8A8Unorm,      vk::Format::eR8G8B8Unorm,     vk::Format::eR8G8Unorm,    vk::Format::eR8Unorm
                };
                return *this;
            }

            [[nodiscard]] std::shared_ptr<ImageResource> create()
            {
                return std::make_shared<ImageResource>(expected_formats);
            }

            std::set<vk::Format> expected_formats;
        };

        explicit ImageResource(std::set<vk::Format> const& expected_formats)
        : m_expected_formats(expected_formats) {}

        bool validate(std::shared_ptr<IOutput> const& incoming) override
        {


            return true;
        }

    private:
        std::set<vk::Format>         m_expected_formats {};
        std::shared_ptr<sdvk::Image> m_resource { nullptr };
    };
}