#pragma once

#include <vector>

#include <robin_hood.h>

#include <rapi/idevice.hpp>
#include <rapi/vulkan/vk_instance.hpp>
#include <rapi/vulkan/vk_struct_chain.hpp>
#include <util/attributes.hpp>

// fwd.
typedef struct VmaAllocator_T* VmaAllocator;

namespace krypton::rapi::vk {
    class Device;

    class PhysicalDevice : public IPhysicalDevice {
        friend class Device;

        Instance* instance;

        std::vector<VkPhysicalDevice> availablePhysicalDevices = {};
        std::vector<VkQueueFamilyProperties2> queueFamilies = {};
        std::vector<uint32_t> presentQueueIndices;

        robin_hood::unordered_set<std::string> availableExtensions;

        VkPhysicalDevice physicalDevice = nullptr;

    public:
        VkPhysicalDeviceProperties properties = {};

        explicit PhysicalDevice(VkPhysicalDevice physicalDevice, Instance* instance) noexcept;

        bool canPresentToWindow(Window* window) override;
        auto createDevice() -> std::unique_ptr<IDevice> override;
        auto getDeviceFeatures(void* pNext = nullptr) -> VkPhysicalDeviceFeatures2;
        auto getDeviceProperties(void* pNext = nullptr) -> VkPhysicalDeviceProperties2;
        void init();
        [[nodiscard]] bool isPortabilityDriver() const override;
        bool meetsMinimumRequirement() override;
        auto supportedDeviceFeatures() -> DeviceFeatures override;
        ALWAYS_INLINE [[nodiscard]] bool supportsExtension(const char* name) const;
    };

    class Device final : public IDevice {
        Instance* instance;
        class PhysicalDevice* physicalDevice;

        VkDevice device = nullptr;
        robin_hood::unordered_set<std::string> enabledExtensions;

        VmaAllocator allocator = nullptr;

        // Queue family indices.
        uint32_t presentationQueueFamily = UINT32_MAX;
        uint32_t transferQueueFamily = UINT32_MAX;

        void determineQueues(std::vector<VkDeviceQueueCreateInfo>& deviceQueues, std::vector<std::vector<float>>& priorities);

    public:
        explicit Device(Instance* instance, vk::PhysicalDevice* pPhysicalDevice, DeviceFeatures features) noexcept;

        auto create() -> VkResult;
        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createFence() -> std::shared_ptr<IFence> override;
        auto createPipeline() -> std::shared_ptr<IPipeline> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createSemaphore() -> std::shared_ptr<ISemaphore> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createSwapchain(Window* window) -> std::shared_ptr<ISwapchain> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        void destroy() override;

        auto getDeviceName() -> std::string_view override {
            ZoneScoped;
            return physicalDevice->properties.deviceName;
        }

        ALWAYS_INLINE [[nodiscard]] inline auto getAllocator() const -> VmaAllocator {
            return allocator;
        }

        ALWAYS_INLINE [[nodiscard]] inline auto getHandle() const -> VkDevice {
            return device;
        }

        ALWAYS_INLINE [[nodiscard]] inline auto getInstance() const -> Instance* {
            return instance;
        }

        ALWAYS_INLINE [[nodiscard]] inline auto getProperties() const -> const VkPhysicalDeviceProperties& {
            return physicalDevice->properties;
        }

        ALWAYS_INLINE [[nodiscard]] inline auto getPhysicalDevice() const -> VkPhysicalDevice {
            return physicalDevice->physicalDevice;
        }

        auto getPresentationQueue() -> std::shared_ptr<IQueue> override;

        [[nodiscard]] bool isExtensionEnabled(const char* extensionName) const;

        ALWAYS_INLINE [[nodiscard]] inline bool isHeadless() const noexcept override {
            return instance->isHeadless();
        }

        ALWAYS_INLINE inline void setDebugUtilsName(VkObjectType objectType, const uint64_t& handle, const char* string) {
            ZoneScoped;
#ifdef KRYPTON_DEBUG
            if (vkSetDebugUtilsObjectNameEXT == nullptr || handle == 0) {
                return;
            }

            VkDebugUtilsObjectNameInfoEXT info = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .objectType = objectType,
                .objectHandle = handle,
                .pObjectName = string,
            };
            vkSetDebugUtilsObjectNameEXT(device, &info);
#endif
        }
    };
} // namespace krypton::rapi::vk
