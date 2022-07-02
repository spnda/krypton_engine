#pragma once

#include <string>

#include <rapi/itexture.hpp>
#include <rapi/vulkan/vma.hpp>

namespace krypton::rapi::vk {
    class Texture : public ITexture {
        class Device* device;
        VmaAllocator allocator;

        VmaAllocation allocation = nullptr;
        VkImage image = nullptr;
        VkImageView imageView = nullptr;

        std::string name = {};
        ColorEncoding encoding = ColorEncoding::LINEAR;
        TextureFormat format = TextureFormat::RGBA8;
        TextureUsage usage;
        VkComponentMapping mapping = {};

    public:
        explicit Texture(Device* device, VmaAllocator allocator, rapi::TextureUsage usage);
        ~Texture() override = default;

        void create(TextureFormat format, uint32_t width, uint32_t height) override;
        void setColorEncoding(ColorEncoding encoding) override;
        void setName(std::string_view name) override;
        void setSwizzling(SwizzleChannels swizzle) override;
        void uploadTexture(std::span<std::byte> data) override;
    };
} // namespace krypton::rapi::vk
