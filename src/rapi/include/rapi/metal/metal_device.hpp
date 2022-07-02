#pragma once

#include <string>

#include <Metal/MTLDevice.hpp>

#include <rapi/idevice.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::metal {
    class Device : public IDevice {
        friend class ::krypton::rapi::MetalBackend;

        MTL::Device* device = nullptr;
        std::string name;

    public:
        explicit Device(MTL::Device* device, DeviceFeatures features) noexcept;

        ~Device() override = default;

        auto createBuffer() -> std::shared_ptr<IBuffer> override;
        auto createRenderPass() -> std::shared_ptr<IRenderPass> override;
        auto createSampler() -> std::shared_ptr<ISampler> override;
        auto createShaderFunction(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType type,
                                  krypton::shaders::ShaderStage stage) -> std::shared_ptr<IShader> override;
        auto createShaderParameter() -> std::shared_ptr<IShaderParameter> override;
        auto createTexture(rapi::TextureUsage usage) -> std::shared_ptr<ITexture> override;
        auto getDeviceName() -> std::string_view override;
    };
} // namespace krypton::rapi::metal
