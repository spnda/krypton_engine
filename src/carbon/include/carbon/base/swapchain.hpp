#pragma once

#include <memory>

#include <carbon/resource/image.hpp>
#include <carbon/vulkan.hpp>

namespace carbon {
    class Device;
    class Instance;
    class Queue;
    class Semaphore;
    class Swapchain;

    class SwapchainImage : public Image {
        friend class carbon::Swapchain;

        explicit SwapchainImage();
        explicit SwapchainImage(std::shared_ptr<carbon::Device> device, VkExtent2D extent);

        void create(VkImage image, VkFormat newFormat);
        void destroy() override;
    };

    class Swapchain {
        carbon::Instance* instance;
        std::shared_ptr<carbon::Device> device;

        VkSwapchainKHR swapchain = nullptr;

        VkSurfaceCapabilitiesKHR capabilities = {};
        std::vector<VkSurfaceFormatKHR> formats = {};
        std::vector<VkPresentModeKHR> presentModes = {};

        VkExtent2D chooseSwapExtent(VkExtent2D windowExtent);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat();
        VkPresentModeKHR chooseSwapPresentMode();
        void querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    public:
        VkSurfaceFormatKHR surfaceFormat = {};
        VkExtent2D imageExtent = {};

        uint32_t imageCount = 0;
        std::vector<std::unique_ptr<carbon::SwapchainImage>> swapchainImages = {};

        Swapchain(carbon::Instance* instance, std::shared_ptr<carbon::Device> device);

        /**
         * Creates a new swapchain. If a swapchain already exists, we
         * re-use it to create a new one.
         */
        bool create(VkSurfaceKHR surface, VkExtent2D windowExtent);
        void destroy();

        [[nodiscard]] auto acquireNextImage(std::shared_ptr<carbon::Semaphore> presentCompleteSemaphore, uint32_t* imageIndex) const
            -> VkResult;
        [[nodiscard]] auto queuePresent(std::shared_ptr<carbon::Queue> queue, uint32_t imageIndex,
                                        std::shared_ptr<carbon::Semaphore> waitSemaphore) const -> VkResult;
        [[nodiscard]] auto getFormat() const -> VkFormat;
        [[nodiscard]] auto getExtent() const -> VkExtent2D;
    };
} // namespace carbon
