#include <carbon/resource/storageimage.hpp>

carbon::StorageImage::StorageImage(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, VkExtent2D windowExtent)
    : carbon::Image(std::move(device), allocator, windowExtent, "storageImage") {}

void carbon::StorageImage::create() { carbon::Image::create(imageFormat, imageUsage, VK_IMAGE_LAYOUT_UNDEFINED); }

void carbon::StorageImage::recreateImage(VkExtent2D windowExtent) {
    destroy();
    imageExtent = windowExtent;
    carbon::Image::create(imageFormat, imageUsage, VK_IMAGE_LAYOUT_UNDEFINED);
}

void carbon::StorageImage::changeLayout(carbon::CommandBuffer* commandBuffer, VkImageLayout newLayout, VkPipelineStageFlags srcStage,
                                        VkPipelineStageFlags dstStage) {
    carbon::Image::changeLayout(commandBuffer, newLayout, subresourceRange, srcStage, dstStage);
}
