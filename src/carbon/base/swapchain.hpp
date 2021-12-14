#pragma once

#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

#include "../context.hpp"

namespace carbon {
    class Swapchain {
        const carbon::Context& ctx;

        VkSurfaceKHR surface;

        PFN_vkAcquireNextImageKHR vkAcquireNextImage;
        PFN_vkDestroySurfaceKHR vkDestroySurface;

        std::vector<VkImage> getImages();
        std::vector<VkImageView> getImageViews();
        
    public:
        vkb::Swapchain swapchain = {};

        std::vector<VkImage> images = {};
        std::vector<VkImageView> views = {};

        Swapchain(const Context& context, VkSurfaceKHR surface);

        // Creates a new swapchain.
        // If a swapchain already exists, we re-use it and
        // later destroy it.
        bool create(const carbon::Device& device);

        void destroy();

        VkResult acquireNextImage(const carbon::Semaphore& presentCompleteSemaphore, uint32_t* imageIndex) const;

        VkResult queuePresent(carbon::Queue& queue, uint32_t imageIndex, carbon::Semaphore& waitSemaphore) const;

        [[nodiscard]] VkFormat getFormat() const;
        [[nodiscard]] VkExtent2D getExtent() const;
    };
} // namespace carbon
