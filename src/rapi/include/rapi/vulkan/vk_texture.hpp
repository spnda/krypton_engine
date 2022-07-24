#pragma once

#include <string>

#include <rapi/itexture.hpp>
#include <rapi/vulkan/vma.hpp>

namespace krypton::rapi::vk {
    VkFormat getVulkanFormat(TextureFormat format);
    VkImageUsageFlags getVulkanImageUsage(TextureUsage usage);

    class Texture : public ITexture {
    protected:
        class Device* device;

        VmaAllocation allocation = nullptr;
        VmaAllocationInfo allocationInfo = {};
        VkImage image;
        VkImageView imageView;

        std::string name = {};
        TextureFormat format = TextureFormat::Invalid;
        TextureUsage usage;
        VkComponentMapping mapping = {};

        explicit Texture(Device* device, rapi::TextureUsage usage, VkImage image, VkImageView imageView);

    public:
        explicit Texture(Device* device, rapi::TextureUsage usage);
        ~Texture() override = default;

        void create(TextureFormat format, uint32_t width, uint32_t height) override;
        void destroy() override;
        [[nodiscard]] auto getHandle() const noexcept -> VkImage;
        [[nodiscard]] auto getView() const noexcept -> VkImageView;
        void setName(std::string_view name) override;
        void setSwizzling(SwizzleChannels swizzle) override;
        void uploadTexture(std::span<std::byte> data) override;
    };

    class SwapchainTexture final : public Texture {
        using Texture::create;

    public:
        explicit SwapchainTexture(Device* device, rapi::TextureUsage usage);
        SwapchainTexture(const SwapchainTexture& other) = default;
        ~SwapchainTexture() override = default;

        void setImage(VkImage image, VkImageView view) noexcept;
    };
} // namespace krypton::rapi::vk
