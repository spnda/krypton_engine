#include "image.hpp"
#include "../utils.hpp"

carbon::Image::Image(const Context& context, const VkExtent2D extent, std::string name)
        : ctx(context), imageExtent(extent), name(std::move(name)) {
}

carbon::Image::operator VkImage() const {
    return this->image;
}

carbon::Image& carbon::Image::operator=(const carbon::Image& newImage) {
    if (this == &newImage) return *this;
    this->image = newImage.image;
    this->imageExtent = newImage.imageExtent;
    this->imageView = newImage.imageView;
    this->allocation = newImage.allocation;
    return *this;
}

void carbon::Image::create(const VkFormat newFormat, const VkImageUsageFlags usageFlags, const VkImageLayout initialLayout) {
    currentLayouts[0] = initialLayout;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;

    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = newFormat;
    imageCreateInfo.extent = { imageExtent.width, imageExtent.height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = usageFlags;
    imageCreateInfo.initialLayout = currentLayouts[0];

    VmaAllocationCreateInfo allocationInfo = {};
    allocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(ctx.vmaAllocator, &imageCreateInfo, &allocationInfo, &image, &allocation, nullptr);

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;

    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.format = newFormat;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(ctx.device, &imageViewCreateInfo, nullptr, &imageView);
    ctx.setDebugUtilsName(image, name);
}

void carbon::Image::create(VkImageCreateInfo* imageCreateInfo, VkImageViewCreateInfo* viewCreateInfo, VkImageLayout initialLayout) {
    currentLayouts[0] = initialLayout;

    VmaAllocationCreateInfo allocationInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    auto result = vmaCreateImage(ctx.vmaAllocator, imageCreateInfo, &allocationInfo, &image, &allocation, nullptr);
    checkResult(ctx, result, "Failed to create image");

    viewCreateInfo->image = image;
    result = vkCreateImageView(ctx.device, viewCreateInfo, nullptr, &imageView);
    checkResult(ctx, result, "Failed to create imageView");
    ctx.setDebugUtilsName(image, name);
}

void carbon::Image::copyImage(VkCommandBuffer cmdBuffer, VkImage destination, VkImageLayout destinationLayout) {
    VkImageCopy copyRegion = {
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .srcOffset = { 0, 0, 0 },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstOffset = { 0, 0, 0 },
        .extent = getImageSize3d(),
    };
    vkCmdCopyImage(cmdBuffer, this->image, this->getImageLayout(), destination, destinationLayout, 1, &copyRegion);
}

void carbon::Image::destroy() {
    vkDestroyImageView(ctx.device, imageView, nullptr);
    vmaDestroyImage(ctx.vmaAllocator, image, allocation);
    imageView = nullptr;
    image = nullptr;
}

VkDescriptorImageInfo carbon::Image::getDescriptorImageInfo() {
    return {
        .imageView = getImageView(),
        .imageLayout = getImageLayout(),
    };
}

VkImageView carbon::Image::getImageView() const {
    return imageView;
}

VkExtent2D carbon::Image::getImageSize() const {
    return imageExtent;
}

VkExtent3D carbon::Image::getImageSize3d() const {
    return {
        .width = getImageSize().width,
        .height = getImageSize().height,
        .depth = 1,
    };
}

VkImageLayout carbon::Image::getImageLayout() {
    return currentLayouts[0];
}

void carbon::Image::changeLayout(
        VkCommandBuffer commandBuffer,
        const VkImageLayout newLayout,
        const VkImageSubresourceRange subresourceRange,
        const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage) {
    carbon::Image::changeLayout(image, commandBuffer, currentLayouts[subresourceRange.baseMipLevel], newLayout, srcStage, dstStage, subresourceRange);
    currentLayouts[subresourceRange.baseMipLevel] = newLayout;
}

void carbon::Image::changeLayout(VkImage image, VkCommandBuffer commandBuffer,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                             VkImageSubresourceRange subresourceRange) {
    uint32_t srcAccessMask, dstAccessMask;
    switch (srcStage) {
        default:
            srcAccessMask = 0;
            break;
        case VK_PIPELINE_STAGE_TRANSFER_BIT:
            srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR:
        case VK_ACCESS_SHADER_WRITE_BIT:
            srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
    }
    switch (dstStage) {
        default:
            dstAccessMask = 0;
            break;
        case VK_PIPELINE_STAGE_TRANSFER_BIT:
            dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR:
        case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
            dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
    }

    VkImageMemoryBarrier imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage,
        dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageBarrier);
}
