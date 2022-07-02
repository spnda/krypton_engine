#pragma once

#include <vector>

#include <rapi/idevice.hpp>

// fwd.
typedef struct VmaAllocator_T* VmaAllocator;

namespace krypton::rapi::vk {
    class Device : public IDevice {
        class Instance* instance = nullptr;

        std::vector<VkPhysicalDevice> availablePhysicalDevices = {};
        std::vector<VkExtensionProperties> availableExtensions = {};
        std::vector<VkQueueFamilyProperties2> queueFamilies = {};
        VkPhysicalDeviceProperties properties = {};

        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;

        VmaAllocator allocator = nullptr;
        VkSurfaceKHR surface = nullptr;

        void determineQueues(std::vector<VkDeviceQueueCreateInfo>& deviceQueues, std::vector<std::vector<float>>& priorities);
        void enableDesiredExtensions(std::vector<const char*>& deviceExtensions);
        VkPhysicalDeviceProperties2 getDeviceProperties(void* pNext = nullptr);
        bool isExtensionSupported(const char* extensionName);

    public:
        explicit Device(Instance* instance, DeviceFeatures features) noexcept;

        auto create(VkSurfaceKHR surface) -> VkResult;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        auto getDeviceName() -> std::string_view override;
        auto getHandle() -> VkDevice;
    };
} // namespace krypton::rapi::vk
