#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/vulkan/vma.hpp>
#include <util/bits.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::vk {
    VkFormat getVulkanFormat(TextureFormat format, ColorEncoding encoding) {
        switch (format) {
            case TextureFormat::RGBA8: {
                if (encoding == ColorEncoding::LINEAR)
                    return VK_FORMAT_R8G8B8A8_UNORM;
                return VK_FORMAT_R8G8B8A8_SRGB;
            }
            case TextureFormat::RGBA16: {
                if (encoding == ColorEncoding::LINEAR)
                    return VK_FORMAT_R16G16B16A16_UNORM;
                return VK_FORMAT_UNDEFINED;
            }
            case TextureFormat::R8: {
                if (encoding == ColorEncoding::LINEAR)
                    return VK_FORMAT_R8_UNORM;
                return VK_FORMAT_R8_SRGB;
            }
            default:
                return VK_FORMAT_UNDEFINED;
        }
    }
} // namespace krypton::rapi::vk

kr::vk::Texture::Texture(Device* device, VmaAllocator allocator, rapi::TextureUsage usage)
    : device(device), allocator(allocator), usage(usage) {}

void kr::vk::Texture::create(TextureFormat newFormat, uint32_t width, uint32_t height) {
    ZoneScoped;
    format = newFormat;

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

    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format = getVulkanFormat(format, encoding),
        .extent = {
            .width = width,
            .height = height,
        },
        .usage = usageFlags,
    };

    VmaAllocationCreateInfo allocationInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    vmaCreateImage(allocator, &imageInfo, &allocationInfo, &image, &allocation, nullptr);
}

void kr::vk::Texture::setColorEncoding(ColorEncoding newEncoding) {
    encoding = newEncoding;
}

void kr::vk::Texture::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (image == nullptr || vkSetDebugUtilsObjectNameEXT == nullptr)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_IMAGE,
        .objectHandle = reinterpret_cast<const uint64_t&>(image),
        .pObjectName = name.c_str(),
    };
    vkSetDebugUtilsObjectNameEXT(device->getHandle(), &info);
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
