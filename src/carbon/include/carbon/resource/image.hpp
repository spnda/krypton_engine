#pragma once

#include <map>
#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace carbon {
    class CommandBuffer;
    class Device;

    class Image {
        std::string name;

        VmaAllocator allocator = nullptr;
        VmaAllocation allocation = nullptr;
        VkImage image = nullptr;
        VkImageView imageView = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;

    protected:
        std::shared_ptr<carbon::Device> device;
        VkExtent2D imageExtent = { 0, 0 };
        std::map<uint32_t, VkImageLayout> currentLayouts = {
            { 0, VK_IMAGE_LAYOUT_UNDEFINED }
        };

    public:
        Image(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, VkExtent2D extent, std::string name = {});

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
            std::shared_ptr<carbon::CommandBuffer> cmdBuffer,
            VkImageLayout newLayout,
            const VkImageSubresourceRange& subresourceRange,
            VkPipelineStageFlags srcStage,
            VkPipelineStageFlags dstStage);

        static void changeLayout(
            VkImage image,
            std::shared_ptr<carbon::CommandBuffer> cmdBuffer,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
            const VkImageSubresourceRange& subresourceRange
        );

        operator VkImage() const;
    };
}
