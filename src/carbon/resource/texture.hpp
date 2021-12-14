#pragma once

#include <vulkan/vulkan.h>

#include "image.hpp"

namespace carbon {
    class Context;

    class Texture : public Image {
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB; // This is the format STB uses.

        uint32_t mips = 1;
        std::string name;
        VkSampler sampler = nullptr;

    public:
        static const VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        Texture(const carbon::Context& context, VkExtent2D imageSize, std::string name = "texture");

        explicit operator VkImageView() const;
        Texture& operator=(const Texture& newImage);

        void createTexture(VkFormat newFormat = VK_FORMAT_R8G8B8A8_SRGB, uint32_t mipLevels = 1, uint32_t arrayLayers = 1);
        void generateMipmaps(VkCommandBuffer cmdBuffer);

        [[nodiscard]] VkSampler getSampler() const;

        static bool formatSupportsBlit(const carbon::Context& ctx, VkFormat format);
    };
}
