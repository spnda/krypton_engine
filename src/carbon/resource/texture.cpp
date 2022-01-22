#include <utility>

#include <carbon/base/command_buffer.hpp>
#include <carbon/base/device.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/resource/texture.hpp>
#include <carbon/utils.hpp>

carbon::Texture::Texture(std::shared_ptr<carbon::Device> device, VmaAllocator allocator, const VkExtent2D imageSize, std::string name)
    : carbon::Image(std::move(device), allocator, imageSize, std::move(name)) {}

carbon::Texture& carbon::Texture::operator=(const Texture& newImage) {
    sampler = newImage.sampler;
    name = newImage.name;
    carbon::Image::operator=(newImage);
    return *this;
}

void carbon::Texture::createTexture(VkFormat newFormat, uint32_t mipLevels, uint32_t arrayLayers) {
    mips = mipLevels;
    imageFormat = newFormat;

    VkImageCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = imageFormat,
        .extent = { imageExtent.width, imageExtent.height, 1 },
        .mipLevels = mips,
        .arrayLayers = arrayLayers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = imageUsage,
        .initialLayout = currentLayouts[0],
    };
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = imageFormat,
        .components = mapping,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mips,
            .baseArrayLayer = 0,
            .layerCount = arrayLayers,
        },
    };
    carbon::Image::create(&createInfo, &imageViewCreateInfo, VK_IMAGE_LAYOUT_UNDEFINED);

    VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,

        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
    };
    auto result = vkCreateSampler(*device, &samplerCreateInfo, nullptr, &sampler);
    checkResult(result, "Failed to create sampler");
}

void carbon::Texture::generateMipmaps(carbon::CommandBuffer* cmdBuffer) {
    // Check if format properties support blitting on this hardware.
    if (!formatSupportsBlit(device, imageFormat))
        return;

    uint32_t mipWidth = imageExtent.width;
    uint32_t mipHeight = imageExtent.height;
    for (uint32_t i = 1; i < mips; i++) {
        // Transition to TRANSFER_SRC_OPTIMAL layout and make sure to not clash with the blit command.
        changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 1, 0, 1 },
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Blit the image.
        VkImageBlit blit = {
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { static_cast<int32_t>(mipWidth), static_cast<int32_t>(mipHeight), 1 };
        blit.dstOffsets[0] = { 0, 0, 0 };
        // We want half the size of the previous mip for this mip level.
        blit.dstOffsets[1] = { mipWidth > 1 ? static_cast<int32_t>(mipWidth / 2) : 1,
                               mipHeight > 1 ? static_cast<int32_t>(mipHeight / 2) : 1, 1 };
        vkCmdBlitImage(*cmdBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                       VK_FILTER_LINEAR);

        // Make sure this mip is not used again until this buffer is finished.
        changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 1, 0, 1 },
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

        // Prepare the size for the next mip
        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    // Also transition the last mip to SHADER_READ_ONLY_OPTIMAL.
    // This doesn't happen in the loop cause the last mip never gets blit from.
    // Also covers the case if we don't want to generate any mips for this texture.
    changeLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, { VK_IMAGE_ASPECT_COLOR_BIT, mips - 1, 1, 0, 1 },
                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);
}

VkDescriptorImageInfo carbon::Texture::getDescriptorImageInfo() {
    return VkDescriptorImageInfo {
        .sampler = sampler,
        .imageView = getImageView(),
        .imageLayout = getImageLayout(),
    };
}

VkSampler carbon::Texture::getSampler() const { return sampler; }

void carbon::Texture::setComponentMapping(VkComponentMapping newMapping) { mapping = newMapping; }

bool carbon::Texture::formatSupportsBlit(std::shared_ptr<carbon::Device> device, VkFormat format) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(*device->getPhysicalDevice(), format, &formatProperties);
    return isFlagSet(formatProperties.optimalTilingFeatures, VK_FORMAT_FEATURE_BLIT_DST_BIT);
}

carbon::Texture::operator VkImageView() const { return getImageView(); }
