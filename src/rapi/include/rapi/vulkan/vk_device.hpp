#pragma once

#include <vector>

#include <rapi/idevice.hpp>
#include <util/attributes.hpp>

// fwd.
typedef struct VmaAllocator_T* VmaAllocator;

namespace krypton::rapi::vk {
    class Device final : public IDevice {
        class Instance* instance;
        class Window* window;

        std::vector<VkPhysicalDevice> availablePhysicalDevices = {};
        std::vector<VkExtensionProperties> availableExtensions = {};
        std::vector<VkQueueFamilyProperties2> queueFamilies = {};
        VkPhysicalDeviceProperties properties = {};

        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;

        VmaAllocator allocator = nullptr;
        VkSurfaceKHR surface = nullptr;

        // Queue family indices.
        uint32_t presentationQueueFamily = UINT32_MAX;
        uint32_t transferQueueFamily = UINT32_MAX;

        void determineQueues(std::vector<VkDeviceQueueCreateInfo>& deviceQueues, std::vector<std::vector<float>>& priorities);
        void enableDesiredExtensions(std::vector<const char*>& deviceExtensions);
        auto getDeviceFeatures(void* pNext = nullptr) -> VkPhysicalDeviceFeatures2;
        auto getDeviceProperties(void* pNext = nullptr) -> VkPhysicalDeviceProperties2;
        bool isExtensionSupported(const char* extensionName);

    public:
        explicit Device(Instance* instance, Window* window, DeviceFeatures features) noexcept;

        auto create() -> VkResult;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createSemaphore() -> std::shared_ptr<ISemaphore> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createSwapchain(Window* window) -> std::shared_ptr<ISwapchain> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        auto getDeviceName() -> std::string_view override;
        [[nodiscard]] auto getHandle() -> VkDevice;
        [[nodiscard]] auto getInstance() -> Instance*;
        [[nodiscard]] auto getPhysicalDevice() -> VkPhysicalDevice;
        auto getPresentationQueue() -> std::shared_ptr<IQueue> override;
        ALWAYS_INLINE void setDebugUtilsName(VkObjectType objectType, const uint64_t& handle, const char* string);
    };
} // namespace krypton::rapi::vk
