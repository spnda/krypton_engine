#pragma once

#include <rapi/iswapchain.hpp>
#include <rapi/itexture.hpp>

// fwd.
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
typedef struct VkSwapchainKHR_T* VkSwapchainKHR;

namespace krypton::rapi::vk {
    class Swapchain final : public ISwapchain {
        class Window* window;
        class Device* device;
        VkSurfaceKHR surface = nullptr;
        VkSwapchainKHR swapchain = nullptr;

        // Properties & Capabilities
        VkSurfaceCapabilitiesKHR capabilities = {};
        std::vector<VkPresentModeKHR> presentModes;
        std::vector<VkSurfaceFormatKHR> formats;

        // Images
        VkImageUsageFlags imageFlags;
        uint32_t imageCount = 0;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

        // Current state
        std::string name;
        uint32_t imageIndex = 0;
        VkExtent2D currentExtent = {};
        VkPresentModeKHR currentPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkSurfaceFormatKHR surfaceFormat = {};

        auto calculateSurfaceExtent() -> VkExtent2D;
        auto choosePresentMode() -> VkPresentModeKHR;
        auto chooseSurfaceFormat() -> VkSurfaceFormatKHR;
        void recreateSwapchain();

    public:
        explicit Swapchain(Device* device, Window* window, VkSurfaceKHR surface);
        ~Swapchain() override = default;

        void create(TextureUsage usage) override;
        void destroy() override;
        auto getDrawable() -> ITexture* override;
        auto getImageCount() -> uint32_t override;
        auto nextImage(ISemaphore* signal, uint32_t* imageIndex, bool* needsResize) -> std::unique_ptr<ITexture> override;
        void present(IQueue* queue, ISemaphore* wait, uint32_t* imageIndex, bool* needsResize) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
