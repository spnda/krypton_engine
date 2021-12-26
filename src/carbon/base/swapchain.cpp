#include <algorithm>
#include <cstdint>

#include "swapchain.hpp"
#include "semaphore.hpp"
#include "../utils.hpp"

carbon::Swapchain::Swapchain(std::shared_ptr<carbon::Instance> instance,
                             std::shared_ptr<carbon::Device> device)
        : instance(std::move(instance)), device(std::move(device)) {
    swapchain = nullptr;
}

bool carbon::Swapchain::create(VkSurfaceKHR surface, VkExtent2D windowExtent) {
    querySwapChainSupport(device->getVkbDevice().physical_device, surface);
    surfaceFormat = chooseSwapSurfaceFormat();
    imageExtent = chooseSwapExtent(windowExtent);
    auto presentMode = chooseSwapPresentMode();

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,

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
    auto res = device->vkCreateSwapchainKHR(*device, &createInfo, nullptr, &newSwapchain);
    checkResult(res, "Failed to create swapchain");
    swapchain = newSwapchain;

    // Get the swapchain images
    device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, swapchainImages.data());
    
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

        res = vkCreateImageView(*device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
        checkResult(res, "Failed to create swapchain image view");
    }

    return true;
}

void carbon::Swapchain::destroy() {
    for (auto& view : swapchainImageViews) {
        vkDestroyImageView(*device, view, nullptr);
        view = nullptr;
    }
    vkDestroySwapchainKHR(*device, swapchain, nullptr);
    swapchain = nullptr;
}

VkResult carbon::Swapchain::acquireNextImage(std::shared_ptr<carbon::Semaphore> presentCompleteSemaphore, uint32_t* imageIndex) const {
    return device->vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, presentCompleteSemaphore->getHandle(), (VkFence)nullptr, imageIndex);
}

VkResult carbon::Swapchain::queuePresent(std::shared_ptr<carbon::Queue> queue, uint32_t imageIndex, std::shared_ptr<carbon::Semaphore> waitSemaphore) const {
    return queue->present(imageIndex, swapchain, waitSemaphore);
}

VkFormat carbon::Swapchain::getFormat() const {
    return surfaceFormat.format;
}

VkExtent2D carbon::Swapchain::getExtent() const {
    return imageExtent;
}

VkExtent2D carbon::Swapchain::chooseSwapExtent(VkExtent2D windowExtent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = windowExtent;
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

void carbon::Swapchain::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

    uint32_t formatCount;
    instance->vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        instance->vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    instance->vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        instance->vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
    }
}
