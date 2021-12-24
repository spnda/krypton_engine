#pragma once

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#include "../context.hpp"

namespace carbon {
    class Swapchain {
        const carbon::Context& ctx;

        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;

        VkSwapchainKHR swapchain = nullptr;

        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        VkExtent2D chooseSwapExtent();
        VkSurfaceFormatKHR chooseSwapSurfaceFormat();
        VkPresentModeKHR chooseSwapPresentMode();
        void querySwapChainSupport(VkPhysicalDevice device);
        
    public:
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D imageExtent;

        uint32_t imageCount;
        std::vector<VkImage> swapchainImages = {};
        std::vector<VkImageView> swapchainImageViews = {};

        Swapchain(const Context& context);

        /**
         * Creates a new swapchain. If a swapchain already exists, we
         * re-use it to create a new one.
         */
        bool create();
        void destroy();

        VkResult acquireNextImage(const carbon::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex) const;

        VkResult queuePresent(carbon::Queue& queue, uint32_t imageIndex, carbon::Semaphore& waitSemaphore) const;

        [[nodiscard]] VkFormat getFormat() const;
        [[nodiscard]] VkExtent2D getExtent() const;
    };
} // namespace carbon
