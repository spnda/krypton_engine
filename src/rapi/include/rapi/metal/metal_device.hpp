#pragma once

#include <string>

#include <Metal/MTLDevice.hpp>

#include <rapi/idevice.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class Device;

    class PhysicalDevice : public IPhysicalDevice {
        friend class Device;

        MTL::Device* device;

    public:
        explicit PhysicalDevice(MTL::Device* device) noexcept;

        bool canPresentToWindow(Window* window) override;
        auto createDevice() -> std::unique_ptr<IDevice> override;
        bool isPortabilityDriver() const override;
        bool meetsMinimumRequirement() override;
        auto supportedDeviceFeatures() -> DeviceFeatures override;
    };

    class Device final : public IDevice {
        friend class ::krypton::rapi::MetalBackend;

        MTL::Device* device;
        std::string name;

    public:
        explicit Device(MTL::Device* device, DeviceFeatures features) noexcept;

        ~Device() override = default;

        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createPipeline() -> std::shared_ptr<IPipeline> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createSemaphore() -> std::shared_ptr<ISemaphore> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createSwapchain(Window* window) -> std::shared_ptr<ISwapchain> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        auto getDeviceName() -> std::string_view override;
        auto getPresentationQueue() -> std::shared_ptr<IQueue> override;
        [[nodiscard]] bool isHeadless() const noexcept override;
    };
} // namespace krypton::rapi::mtl
