#include <memory>

#include <carbon/base/command_buffer.hpp>
#include <carbon/base/device.hpp>
#include <carbon/resource/image.hpp>
#include <carbon/utils.hpp>

carbon::Image::Image(std::shared_ptr<carbon::Device> context, VmaAllocator allocator, const VkExtent2D extent, std::string name)
    : device(std::move(context)), allocator(allocator), imageExtent(extent), name(std::move(name)) {}

carbon::Image::operator VkImage() const { return this->handle; }

carbon::Image& carbon::Image::operator=(const carbon::Image& newImage) {
    if (this == &newImage)
        return *this;
    this->handle = newImage.handle;
    this->imageExtent = newImage.imageExtent;
    this->imageView = newImage.imageView;
    this->allocation = newImage.allocation;
    return *this;
}

void carbon::Image::create(const VkFormat newFormat, const VkImageUsageFlags usageFlags, const VkImageLayout initialLayout) {
    currentLayouts[0] = initialLayout;
    format = newFormat;

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

    vmaCreateImage(allocator, &imageCreateInfo, &allocationInfo, &handle, &allocation, nullptr);

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.pNext = nullptr;

    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.image = handle;
    imageViewCreateInfo.format = newFormat;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(*device, &imageViewCreateInfo, nullptr, &imageView);
    device->setDebugUtilsName(handle, name);
}

void carbon::Image::create(VkImageCreateInfo* imageCreateInfo, VkImageViewCreateInfo* viewCreateInfo, VkImageLayout initialLayout) {
    currentLayouts[0] = initialLayout;

    VmaAllocationCreateInfo allocationInfo = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    auto result = vmaCreateImage(allocator, imageCreateInfo, &allocationInfo, &handle, &allocation, nullptr);
    checkResult(result, "Failed to create image");

    viewCreateInfo->image = handle;
    result = vkCreateImageView(*device, viewCreateInfo, nullptr, &imageView);
    checkResult(result, "Failed to create imageView");
    device->setDebugUtilsName(handle, name);
}

void carbon::Image::copyImage(carbon::CommandBuffer* cmdBuffer, VkImage destination, VkImageLayout destinationLayout) {
    VkImageCopy copyRegion = {
        .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .srcOffset = { 0, 0, 0 },
        .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .dstOffset = { 0, 0, 0 },
        .extent = getImageSize3d(),
    };
    vkCmdCopyImage(*cmdBuffer, this->handle, this->getImageLayout(), destination, destinationLayout, 1, &copyRegion);
}

void carbon::Image::destroy() {
    vkDestroyImageView(*device, imageView, nullptr);
    if (allocation != nullptr) // Swapchain images have no allocation.
        vmaDestroyImage(allocator, handle, allocation);
    imageView = nullptr;
    handle = nullptr;
}

VkDescriptorImageInfo carbon::Image::getDescriptorImageInfo() {
    return {
        .imageView = getImageView(),
        .imageLayout = getImageLayout(),
    };
}

VkFormat carbon::Image::getImageFormat() const { return format; }

VkImageLayout carbon::Image::getImageLayout() { return currentLayouts[0]; }

VkExtent2D carbon::Image::getImageSize() const { return imageExtent; }

VkExtent3D carbon::Image::getImageSize3d() const {
    return {
        .width = getImageSize().width,
        .height = getImageSize().height,
        .depth = 1,
    };
}

VkImageView carbon::Image::getImageView() const { return imageView; }

void carbon::Image::changeLayout(carbon::CommandBuffer* cmdBuffer, const VkImageLayout newLayout,
                                 const VkImageSubresourceRange& subresourceRange, const VkPipelineStageFlags srcStage,
                                 const VkPipelineStageFlags dstStage) {
    carbon::Image::changeLayout(handle, cmdBuffer, currentLayouts[subresourceRange.baseMipLevel], newLayout, srcStage, dstStage,
                                subresourceRange);
    currentLayouts[subresourceRange.baseMipLevel] = newLayout;
}

void carbon::Image::changeLayout(VkImage image, carbon::CommandBuffer* cmdBuffer, VkImageLayout oldLayout, VkImageLayout newLayout,
                                 VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                 const VkImageSubresourceRange& subresourceRange) {
    uint32_t srcAccessMask, dstAccessMask;
    switch (srcStage) {
        default: srcAccessMask = 0; break;
        case VK_PIPELINE_STAGE_TRANSFER_BIT: srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT; break;
        case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR:
        case VK_ACCESS_SHADER_WRITE_BIT: srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT; break;
    }
    switch (dstStage) {
        default: dstAccessMask = 0; break;
        case VK_PIPELINE_STAGE_TRANSFER_BIT: dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT; break;
        case VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR:
        case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT: dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; break;
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

    cmdBuffer->pipelineBarrier(srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
}
