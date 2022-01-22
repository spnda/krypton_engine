#pragma once

#include <carbon/resource/image.hpp>

namespace carbon {
    /** A helper class to easily create and use a storage image for raytracing */
    class StorageImage : public Image {
        static const VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        static const uint32_t imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        const VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    public:
        explicit StorageImage(std::shared_ptr<carbon::Device>, VmaAllocator allocator, VkExtent2D windowExtent);

        void create();
        void recreateImage(VkExtent2D windowExtent);

        void changeLayout(carbon::CommandBuffer* commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStage,
                          VkPipelineStageFlags dstStage);
    };
} // namespace carbon
