#pragma once

#include <rapi/iswapchain.hpp>
#include <rapi/itexture.hpp>
#include <rapi/vulkan/vk_texture.hpp>

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
        VkImageUsageFlags imageFlags = 0;
        TextureUsage textureUsage = TextureUsage::None;
        uint32_t imageCount = 0;
        std::vector<SwapchainTexture> textures;
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;

        // Current state
        std::string name;
        uint32_t imageIndex = 0;
        VkExtent2D currentExtent = {};
        VkPresentModeKHR currentPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkSurfaceFormatKHR surfaceFormat = {};
        TextureFormat textureFormat = TextureFormat::Invalid;

        auto calculateSurfaceExtent() -> VkExtent2D;
        auto choosePresentMode() -> VkPresentModeKHR;
        void chooseSurfaceFormat();
        void recreateSwapchain();

    public:
        explicit Swapchain(Device* device, Window* window);
        ~Swapchain() override = default;

        void create(TextureUsage usage) override;
        void destroy() override;
        auto getDrawableFormat() -> TextureFormat override;
        auto getDrawable() -> ITexture* override;
        auto getImageCount() -> uint32_t override;
        void nextImage(ISemaphore* signal, bool* needsResize) override;
        void present(IQueue* queue, ISemaphore* wait, bool* needsResize) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
