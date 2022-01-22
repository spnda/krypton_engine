#pragma once

#include <memory>

#include <carbon/resource/image.hpp>
#include <carbon/vulkan.hpp>

namespace carbon {
    class CommandBuffer;
    class Context;

    class Texture : public Image {
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB; // This is the format STB uses.

        uint32_t mips = 1;
        std::string name;
        VkSampler sampler = nullptr;

        VkComponentMapping mapping = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };

    public:
        static const VkImageUsageFlags imageUsage =
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        Texture(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, VkExtent2D imageSize, std::string name = "texture");

        Texture& operator=(const Texture& newImage);

        void createTexture(VkFormat newFormat = VK_FORMAT_R8G8B8A8_SRGB, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);
        void generateMipmaps(carbon::CommandBuffer* cmdBuffer);

        [[nodiscard]] auto getDescriptorImageInfo() -> VkDescriptorImageInfo override;
        [[nodiscard]] VkSampler getSampler() const;
        void setComponentMapping(VkComponentMapping mapping);

        static bool formatSupportsBlit(std::shared_ptr<carbon::Device> device, VkFormat format);

        explicit operator VkImageView() const;
    };
} // namespace carbon
