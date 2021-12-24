#include <algorithm>
#include <cstdint>

#include "swapchain.hpp"
#include "../utils.hpp"

carbon::Swapchain::Swapchain(const Context& context)
        : ctx(context) {
    swapchain = nullptr;
}

bool carbon::Swapchain::create() {
    DEVICE_FUNCTION_POINTER(vkAcquireNextImageKHR, ctx.device)
    DEVICE_FUNCTION_POINTER(vkDestroySurfaceKHR, ctx.device)
    DEVICE_FUNCTION_POINTER(vkGetSwapchainImagesKHR, ctx.device)

    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, ctx.instance)
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfaceFormatsKHR, ctx.instance)
    INSTANCE_FUNCTION_POINTER(vkGetPhysicalDeviceSurfacePresentModesKHR, ctx.instance)

    querySwapChainSupport(ctx.physicalDevice);
    surfaceFormat = chooseSwapSurfaceFormat();
    imageExtent = chooseSwapExtent();
    auto presentMode = chooseSwapPresentMode();

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ctx.surface,

        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = imageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // We need this when switching layouts to copy the storage image.
        
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,

        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = swapchain,
    };

    VkSwapchainKHR newSwapchain = nullptr;
    auto res = vkCreateSwapchainKHR(ctx.device, &createInfo, nullptr, &newSwapchain);
    checkResult(ctx, res, "Failed to create swapchain");
    swapchain = newSwapchain;

    // Get the swapchain images
    vkGetSwapchainImagesKHR(ctx.device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.device, swapchain, &imageCount, swapchainImages.data());
    
    // Get the swapchain image views
    swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < swapchainImages.size(); ++i) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surfaceFormat.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        res = vkCreateImageView(ctx.device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
        checkResult(ctx, res, "Failed to create swapchain image view");
    }

    return true;
}

void carbon::Swapchain::destroy() {
    for (auto& view : swapchainImageViews) {
        vkDestroyImageView(ctx.device, view, nullptr);
        view = nullptr;
    }
    vkDestroySwapchainKHR(ctx.device, swapchain, nullptr);
    swapchain = nullptr;
}

VkResult carbon::Swapchain::acquireNextImage(const carbon::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex) const {
    return vkAcquireNextImageKHR(ctx.device, swapchain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, imageIndex);
}

VkResult carbon::Swapchain::queuePresent(carbon::Queue& queue, uint32_t imageIndex, carbon::Semaphore& waitSemaphore) const {
    return queue.present(imageIndex, swapchain, waitSemaphore);
}

VkFormat carbon::Swapchain::getFormat() const {
    return surfaceFormat.format;
}

VkExtent2D carbon::Swapchain::getExtent() const {
    return imageExtent;
}

VkExtent2D carbon::Swapchain::chooseSwapExtent() {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = { ctx.width, ctx.height };
        actualExtent.width = std::max(capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkSurfaceFormatKHR carbon::Swapchain::chooseSwapSurfaceFormat() {
    /*for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }*/
    return formats[0];
}

VkPresentModeKHR carbon::Swapchain::chooseSwapPresentMode() {
    for (const auto& availablePresentMode : presentModes) {
        /* Best present mode that doesn't result in tearing but still somewhat unlocks framerates */
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; /* Essentially V-Sync */
}

void carbon::Swapchain::querySwapChainSupport(VkPhysicalDevice device) {
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, ctx.surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx.surface, &formatCount, nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, ctx.surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, ctx.surface, &presentModeCount, presentModes.data());
    }
}
