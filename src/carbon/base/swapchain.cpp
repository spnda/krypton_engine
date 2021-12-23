#include "VkBootstrap.h"

#include "swapchain.hpp"
#include "../utils.hpp"

carbon::Swapchain::Swapchain(const Context& context, const VkSurfaceKHR surface)
        : ctx(context), surface(surface) {
}

bool carbon::Swapchain::create(const carbon::Device& device) {
    vkAcquireNextImage = ctx.device.getFunctionAddress<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR");
    vkDestroySurface = ctx.device.getFunctionAddress<PFN_vkDestroySurfaceKHR>("vkDestroySurfaceKHR");
    
    /* We don't call the copy ctor of vkb::Device here as that seems to throw exceptions. */
    const vkb::Device& vkbDevice = device.getVkbDevice();
    vkb::SwapchainBuilder swapchainBuilder(vkbDevice);
    auto buildResult = swapchainBuilder
        .set_old_swapchain(this->swapchain)
        .set_desired_extent(ctx.width, ctx.height)
        .set_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) // We need this when switching layouts to copy the storage image.
        .build();
    vkb::destroy_swapchain(this->swapchain);
    swapchain = getFromVkbResult(buildResult);
    
    images = getImages();
    views = getImageViews();
    return true;
}

void carbon::Swapchain::destroy() {
    swapchain.destroy_image_views(views);
    vkb::destroy_swapchain(this->swapchain);
    
    if (surface != nullptr)
        vkDestroySurface(ctx.instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
}

VkResult carbon::Swapchain::acquireNextImage(const carbon::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex) const {
    return vkAcquireNextImage(ctx.device, swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult carbon::Swapchain::queuePresent(carbon::Queue& queue, uint32_t imageIndex, carbon::Semaphore& waitSemaphore) const {
    return queue.present(imageIndex, swapchain.swapchain, waitSemaphore);
}

VkFormat carbon::Swapchain::getFormat() const {
    return swapchain.image_format;
}

VkExtent2D carbon::Swapchain::getExtent() const {
    return swapchain.extent;
}

std::vector<VkImage> carbon::Swapchain::getImages() {
    return getFromVkbResult(swapchain.get_images());
}

std::vector<VkImageView> carbon::Swapchain::getImageViews() {
    return getFromVkbResult(swapchain.get_image_views());
}
