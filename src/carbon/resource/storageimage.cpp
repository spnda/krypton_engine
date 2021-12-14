#include "storageimage.hpp"

carbon::StorageImage::StorageImage(const carbon::Context& context)
        : carbon::Image(context, { context.width, context.height }, "storageImage") {

}

void carbon::StorageImage::create() {
    carbon::Image::create(imageFormat, imageUsage, VK_IMAGE_LAYOUT_UNDEFINED);
}

void carbon::StorageImage::recreateImage() {
    destroy();
    imageExtent = { ctx.width, ctx.height };
    carbon::Image::create(imageFormat, imageUsage, VK_IMAGE_LAYOUT_UNDEFINED);
}

void carbon::StorageImage::changeLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
    carbon::Image::changeLayout(commandBuffer, newLayout, subresourceRange, srcStage, dstStage);
}
