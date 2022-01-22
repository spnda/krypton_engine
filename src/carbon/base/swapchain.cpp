#include <algorithm>
#include <cstdint>
#include <memory>

#include <carbon/base/device.hpp>
#include <carbon/base/instance.hpp>
#include <carbon/base/physical_device.hpp>
#include <carbon/base/semaphore.hpp>
#include <carbon/base/swapchain.hpp>
#include <carbon/utils.hpp>

carbon::SwapchainImage::SwapchainImage(std::shared_ptr<carbon::Device> device, VkExtent2D extent)
    : carbon::Image(device, nullptr, extent) {}

void carbon::SwapchainImage::create(VkImage newImage, VkFormat newFormat) {
    handle = newImage;
    format = newFormat;

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
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
            .layerCount = 1,
        },
    };

    auto res = vkCreateImageView(*device, &imageViewCreateInfo, nullptr, &imageView);
    checkResult(res, "Failed to create swapchain image view");
}

void carbon::SwapchainImage::destroy() {
    vkDestroyImageView(*device, imageView, nullptr);
    imageView = nullptr;
}

carbon::Swapchain::Swapchain(carbon::Instance* instance, std::shared_ptr<carbon::Device> device)
    : instance(instance), device(std::move(device)) {}

bool carbon::Swapchain::create(VkSurfaceKHR surface, VkExtent2D windowExtent) {
    querySwapChainSupport(*device->getPhysicalDevice(), surface);
    surfaceFormat = chooseSwapSurfaceFormat();
    imageExtent = chooseSwapExtent(windowExtent);
    auto presentMode = chooseSwapPresentMode();

    uint32_t minImageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount) {
        minImageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,

        .minImageCount = minImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = imageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT, // We need this when switching layouts to copy the storage image.

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

    // First, destroy the previous swapchain image views
    for (auto& image : swapchainImages)
        image->destroy();

    // Get the swapchain images
    device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, nullptr);
    std::vector<VkImage> vulkanImages(imageCount);
    device->vkGetSwapchainImagesKHR(*device, swapchain, &imageCount, vulkanImages.data());

    // Get the swapchain image views
    swapchainImages.resize(imageCount);
    std::transform(vulkanImages.begin(), vulkanImages.end(), swapchainImages.begin(), [&](VkImage vkImage) {
        // We want the ctor of SwapchainImage to be private and will therefore have to
        // call new as make_unique wouldn't be able to call the private function.
        auto image = std::unique_ptr<carbon::SwapchainImage>(new carbon::SwapchainImage(device, imageExtent));
        image->create(vkImage, surfaceFormat.format);
        return image;
    });

    return true;
}

void carbon::Swapchain::destroy() {
    for (auto& image : swapchainImages) {
        image->destroy();
    }
    vkDestroySwapchainKHR(*device, swapchain, nullptr);
    swapchain = nullptr;
}

VkResult carbon::Swapchain::acquireNextImage(std::shared_ptr<carbon::Semaphore> presentCompleteSemaphore, uint32_t* imageIndex) const {
    return device->vkAcquireNextImageKHR(*device, swapchain, UINT64_MAX, presentCompleteSemaphore->getHandle(), (VkFence) nullptr,
                                         imageIndex);
}

VkResult carbon::Swapchain::queuePresent(std::shared_ptr<carbon::Queue> queue, uint32_t imageIndex,
                                         std::shared_ptr<carbon::Semaphore> waitSemaphore) const {
    return queue->present(imageIndex, swapchain, waitSemaphore);
}

VkFormat carbon::Swapchain::getFormat() const { return surfaceFormat.format; }

VkExtent2D carbon::Swapchain::getExtent() const { return imageExtent; }

VkExtent2D carbon::Swapchain::chooseSwapExtent(VkExtent2D windowExtent) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = windowExtent;
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height =
            std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
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
    auto res = std::find_if(presentModes.begin(), presentModes.end(), [](VkPresentModeKHR presentMode) {
        /* Best present mode that doesn't result in tearing but still somewhat unlocks framerates */
        return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
    });
    return res != presentModes.end() ? *res : VK_PRESENT_MODE_FIFO_KHR;
}

void carbon::Swapchain::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    instance->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

    uint32_t formatCount;
    instance->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        instance->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
    }

    uint32_t presentModeCount;
    instance->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        presentModes.resize(presentModeCount);
        instance->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
    }
}
