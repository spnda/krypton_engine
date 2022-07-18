#pragma once

#include <string>

#include <rapi/itexture.hpp>
#include <rapi/vulkan/vma.hpp>

namespace krypton::rapi::vk {
    VkFormat getVulkanFormat(TextureFormat format);
    VkImageUsageFlags getVulkanImageUsage(TextureUsage usage);

    class Texture final : public ITexture {
        class Device* device;
        VmaAllocator allocator;

        VmaAllocation allocation = nullptr;
        VmaAllocationInfo allocationInfo = {};
        VkImage image = nullptr;
        VkImageView imageView = nullptr;

        std::string name = {};
        TextureFormat format = TextureFormat::RGBA8_SRGB;
        TextureUsage usage;
        VkComponentMapping mapping = {};

    public:
        explicit Texture(Device* device, VmaAllocator allocator, rapi::TextureUsage usage);
        ~Texture() override = default;

        void create(TextureFormat format, uint32_t width, uint32_t height) override;
        [[nodiscard]] auto getHandle() const noexcept -> VkImage;
        [[nodiscard]] auto getView() const noexcept -> VkImageView;
        void setName(std::string_view name) override;
        void setSwizzling(SwizzleChannels swizzle) override;
        void uploadTexture(std::span<std::byte> data) override;
    };
} // namespace krypton::rapi::vk
