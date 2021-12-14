#pragma once

#include <map>
#include <string>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "../context.hpp"

namespace carbon {
    class Image {
        std::string name;

        VmaAllocation allocation = nullptr;
        VkImage image = nullptr;
        VkImageView imageView = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;

    protected:
        const Context& ctx;
        VkExtent2D imageExtent = { 0, 0 };
        std::map<uint32_t, VkImageLayout> currentLayouts = {
            { 0, VK_IMAGE_LAYOUT_UNDEFINED }
        };

    public:
        Image(const Context& context, VkExtent2D extent, std::string name = {});

        operator VkImage() const;
        Image& operator=(const Image& newImage);

        void create(VkFormat newFormat, VkImageUsageFlags usageFlags, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
        void create(VkImageCreateInfo* createInfo, VkImageViewCreateInfo* viewCreateInfo, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);
        void copyImage(VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout);
        /** Destroys the image view, frees all memory and destroys the image. */
        void destroy();

        [[nodiscard]] VkDescriptorImageInfo getDescriptorImageInfo();
        [[nodiscard]] VkImageView getImageView() const;
        [[nodiscard]] VkExtent2D getImageSize() const;
        [[nodiscard]] VkExtent3D getImageSize3d() const;
        [[nodiscard]] VkImageLayout getImageLayout();

        void changeLayout(
            VkCommandBuffer commandBuffer,
            VkImageLayout newLayout,
            VkImageSubresourceRange subresourceRange,
            VkPipelineStageFlags srcStage,
            VkPipelineStageFlags dstStage);

        static void changeLayout(
            VkImage image,
            VkCommandBuffer commandBuffer,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            VkImageSubresourceRange subresourceRange
        );
    };
}
