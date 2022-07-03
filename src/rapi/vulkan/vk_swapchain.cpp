#include <string>

#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/vulkan/vk_queue.hpp>
#include <rapi/vulkan/vk_swapchain.hpp>
#include <rapi/vulkan/vk_sync.hpp>
#include <rapi/vulkan/vk_texture.hpp>
#include <rapi/window.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::vk::Swapchain::Swapchain(Device* device, Window* window, VkSurfaceKHR surface) : window(window), device(device), surface(surface) {}

VkExtent2D kr::vk::Swapchain::calculateSurfaceExtent() {
    ZoneScoped;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        glm::u32vec2 windowExtent = window->getWindowSize();
        VkExtent2D actualExtent = {
            .width = windowExtent.x,
            .height = windowExtent.y,
        };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height =
            std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkPresentModeKHR kr::vk::Swapchain::choosePresentMode() {
    ZoneScoped;
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->getPhysicalDevice(), surface, &presentModeCount, nullptr);
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->getPhysicalDevice(), surface, &presentModeCount, presentModes.data());

    auto res = std::find_if(presentModes.begin(), presentModes.end(), [](auto presentMode) {
        /* Best present mode that doesn't result in tearing but still somewhat unlocks the framerate */
        return presentMode == VK_PRESENT_MODE_MAILBOX_KHR;
    });
    return res != presentModes.end() ? *res : VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR kr::vk::Swapchain::chooseSurfaceFormat() {
    ZoneScoped;
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->getPhysicalDevice(), surface, &formatCount, nullptr);
    formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->getPhysicalDevice(), surface, &formatCount, formats.data());

    /*for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }*/
    return formats[0];
}

void kr::vk::Swapchain::create(TextureUsage usage) {
    ZoneScoped;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->getPhysicalDevice(), surface, &capabilities);

    this->imageFlags = getVulkanImageUsage(usage);
    recreateSwapchain();
}

void kr::vk::Swapchain::destroy() {
    ZoneScoped;
    vkDestroySwapchainKHR(device->getHandle(), swapchain, nullptr);
    swapchain = nullptr;
}

kr::ITexture* kr::vk::Swapchain::getDrawable() {
    images[imageIndex];
    return nullptr;
}

uint32_t kr::vk::Swapchain::getImageCount() {
    ZoneScoped;
    // Has to be called after a swapchain has been created. The Vulkan implementation has the final
    // say on the count of images, we can only give a minimum number.
    return imageCount;
}

std::unique_ptr<kr::ITexture> kr::vk::Swapchain::nextImage(ISemaphore* signal, uint32_t* pImageIndex, bool* needsResize) {
    ZoneScoped;
    VkSemaphore semaphore = signal == nullptr ? nullptr : *dynamic_cast<Semaphore*>(signal)->getHandle();
    auto res = vkAcquireNextImageKHR(device->getHandle(), swapchain, 1'000'000, semaphore, nullptr, pImageIndex);
    imageIndex = *pImageIndex;
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        *needsResize = true;
    else if (res != VK_SUCCESS)
        kl::err("Failed to acquire next image: {}", res);
    return nullptr;
}

void kr::vk::Swapchain::present(IQueue* queue, ISemaphore* wait, uint32_t* pImageIndex, bool* needsResize) {
    ZoneScoped;
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = dynamic_cast<Semaphore*>(wait)->getHandle(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = pImageIndex,
    };
    auto res = vkQueuePresentKHR(dynamic_cast<Queue*>(queue)->getHandle(), &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        *needsResize = true;
    else if (res != VK_SUCCESS)
        kl::err("Failed to present image: {}", res);
}

void kr::vk::Swapchain::recreateSwapchain() {
    ZoneScoped;
    currentExtent = calculateSurfaceExtent();
    currentPresentMode = choosePresentMode();
    surfaceFormat = chooseSurfaceFormat();

    VkSwapchainKHR oldSwapchain = swapchain;
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,

        .minImageCount = std::max(capabilities.minImageCount, std::min(3u, capabilities.maxImageCount)),
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = imageFlags,

        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,

        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = currentPresentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain,
    };
    auto res = vkCreateSwapchainKHR(device->getHandle(), &swapchainInfo, nullptr, &swapchain);
    if (res != VK_SUCCESS)
        kl::err("Failed to create swapchain: {}", res);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<const uint64_t&>(swapchain), name.c_str());

    if (oldSwapchain != nullptr)
        vkDestroySwapchainKHR(device->getHandle(), oldSwapchain, nullptr);

    // Get our swapchain images.
    vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device->getHandle(), swapchain, &imageCount, images.data());

    imageViews.resize(imageCount);
    std::transform(images.begin(), images.end(), imageViews.begin(), [&](VkImage vkImage) {
        ZoneScopedN("std::transform VkImageView");
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vkImage,
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
                .layerCount = 1,
            },
        };

        VkImageView imageView;
        auto res = vkCreateImageView(device->getHandle(), &imageViewCreateInfo, nullptr, &imageView);
        if (res != VK_SUCCESS)
            kl::err("Failed to create swapchain image view: {}", res);

        return imageView;
    });
}

void kr::vk::Swapchain::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (swapchain != nullptr && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<const uint64_t&>(swapchain), name.c_str());
}
