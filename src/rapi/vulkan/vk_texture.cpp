#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/vulkan/vma.hpp>
#include <util/assert.hpp>
#include <util/bits.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::vk {
    VkFormat getVulkanFormat(TextureFormat format) {
        ZoneScoped;
        switch (format) {
            case TextureFormat::RGBA8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::RGBA16_UNORM:
                return VK_FORMAT_R16G16B16A16_UNORM;
            case TextureFormat::A2BGR10_UNORM:
                return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case TextureFormat::R8_UNORM:
                return VK_FORMAT_R8_UNORM;
            case TextureFormat::R8_SRGB:
                return VK_FORMAT_R8_SRGB;
            default:
                return VK_FORMAT_UNDEFINED;
        }
    }

    VkImageUsageFlags getVulkanImageUsage(TextureUsage usage) {
        ZoneScoped;
        VkImageUsageFlags usageFlags = 0;
        if (util::hasBit(usage, TextureUsage::SampledImage)) {
            usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (util::hasBit(usage, TextureUsage::ColorRenderTarget)) {
            usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (util::hasBit(usage, TextureUsage::DepthRenderTarget)) {
            usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        return usageFlags;
    }
} // namespace krypton::rapi::vk

#pragma region vk::Texture
kr::vk::Texture::Texture(Device* device, rapi::TextureUsage usage) : device(device), image(nullptr), imageView(nullptr), usage(usage) {}

kr::vk::Texture::Texture(krypton::rapi::vk::Device* device, rapi::TextureUsage usage, VkImage image, VkImageView imageView)
    : device(device), image(image), imageView(imageView), usage(usage) {}

void kr::vk::Texture::create(TextureFormat newFormat, uint32_t width, uint32_t height) {
    ZoneScoped;
    VERIFY(newFormat != TextureFormat::Invalid);
    format = newFormat;

    VkImageUsageFlags usageFlags = getVulkanImageUsage(usage);

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format = getVulkanFormat(format),
        .extent = {
            .width = width,
            .height = height,
        },
        .usage = usageFlags,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(device->getAllocator(), &imageInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<const uint64_t&>(image), name.c_str());
}

void kr::vk::Texture::destroy() {
    ZoneScoped;
    vkDestroyImageView(device->getHandle(), imageView, nullptr);
    vmaDestroyImage(device->getAllocator(), image, allocation);

    imageView = nullptr;
    image = nullptr;
    allocation = nullptr;
}

VkImage kr::vk::Texture::getHandle() const noexcept {
    return image;
}

VkImageView kr::vk::Texture::getView() const noexcept {
    return imageView;
}

void kr::vk::Texture::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (image != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<const uint64_t&>(image), name.c_str());
}

void kr::vk::Texture::setSwizzling(SwizzleChannels swizzle) {
    ZoneScoped;
    // The VkComponentSwizzle enum is identical to our TextureSwizzle enum, offset by 1
    mapping.r = static_cast<VkComponentSwizzle>(static_cast<uint8_t>(swizzle.r) + 1);
    mapping.g = static_cast<VkComponentSwizzle>(static_cast<uint8_t>(swizzle.g) + 1);
    mapping.b = static_cast<VkComponentSwizzle>(static_cast<uint8_t>(swizzle.b) + 1);
    mapping.a = static_cast<VkComponentSwizzle>(static_cast<uint8_t>(swizzle.a) + 1);
}

void kr::vk::Texture::uploadTexture(std::span<std::byte> data) {
    ZoneScoped;
}
#pragma endregion

#pragma region vk::SwapchainTexture
kr::vk::SwapchainTexture::SwapchainTexture(krypton::rapi::vk::Device* device, rapi::TextureUsage usage) : Texture(device, usage) {}

void kr::vk::SwapchainTexture::setImage(VkImage image, VkImageView view) noexcept {
    this->image = image;
    this->imageView = view;
}
#pragma endregion
