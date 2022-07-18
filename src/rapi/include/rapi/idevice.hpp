#pragma once

#include <span>
#include <string_view>

#include <rapi/itexture.hpp>

// fwd
namespace krypton::shaders {
    enum class ShaderStage : uint16_t;
    enum class ShaderSourceType : uint32_t;
} // namespace krypton::shaders

namespace krypton::rapi {
    class IBuffer;
    class ICommandBuffer;
    class IPipeline;
    class IQueue;
    class IRenderPass;
    class ISampler;
    class ISemaphore;
    class IShader;
    class IShaderParameter;
    class ISwapchain;
    class Window;

    struct DeviceFeatures {
        bool accelerationStructures = false;
        bool bufferDeviceAddress = false;
        bool rayTracing = false;
    };

    class IDevice;

    class IPhysicalDevice {
    public:
        virtual ~IPhysicalDevice() = default;

        [[nodiscard]] virtual bool canPresentToWindow(Window* window) = 0;

        [[nodiscard]] virtual auto createDevice() -> std::unique_ptr<IDevice> = 0;

        // Returns true for Vulkan implementations which are layered above another graphics API.
        // These devices are favored less than native implementations, as they cannot provide a
        // fully conformant Vulkan implementation.
        [[nodiscard]] virtual bool isPortabilityDriver() const = 0;

        // This evaluates if the physical device supports the minimum requirements for the
        // RenderAPI to function.
        [[nodiscard]] virtual bool meetsMinimumRequirement() = 0;

        [[nodiscard]] virtual auto supportedDeviceFeatures() -> DeviceFeatures = 0;
    };

    class IDevice {
    protected:
        DeviceFeatures enabledFeatures;

        explicit IDevice(DeviceFeatures enabledFeatures) : enabledFeatures(enabledFeatures) {}

    public:
        virtual ~IDevice() = default;

        [[nodiscard]] virtual auto createBuffer() -> std::shared_ptr<IBuffer> = 0;

        [[nodiscard]] virtual auto createPipeline() -> std::shared_ptr<IPipeline> = 0;

        [[nodiscard]] virtual auto createRenderPass() -> std::shared_ptr<IRenderPass> = 0;

        [[nodiscard]] virtual auto createSampler() -> std::shared_ptr<ISampler> = 0;

        [[nodiscard]] virtual auto createSemaphore() -> std::shared_ptr<ISemaphore> = 0;

        [[nodiscard]] virtual auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                                        krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> = 0;

        [[nodiscard]] virtual auto createShaderParameter() -> std::shared_ptr<IShaderParameter> = 0;

        [[nodiscard]] virtual auto createSwapchain(Window* window) -> std::shared_ptr<ISwapchain> = 0;

        [[nodiscard]] virtual auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> = 0;

        [[nodiscard]] virtual auto getDeviceName() -> std::string_view = 0;

        [[nodiscard]] virtual auto getPresentationQueue() -> std::shared_ptr<IQueue> = 0;

        // Indicates support for creating and presenting to a swapchain. If false, the device
        // cannot be used to render onto any connected display.
        [[nodiscard]] virtual bool isHeadless() const noexcept = 0;
    };
} // namespace krypton::rapi
