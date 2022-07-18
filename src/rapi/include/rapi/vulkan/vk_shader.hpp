#pragma once

#include <robin_hood.h>
#include <volk.h>

#include <rapi/ishader.hpp>

namespace krypton::rapi::vk {
    class ShaderParameter final : public IShaderParameter {
        class Device* device;

    public:
        explicit ShaderParameter(Device* device);
        ~ShaderParameter() override = default;

        void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void buildParameter() override;
        void destroy() override;
        void setName(std::string_view name) override;
    };

    class Shader final : public IShader {
        class Device* device;

        VkShaderModule shader = nullptr;
        std::string name;
        std::vector<uint32_t> spirv;

        [[nodiscard]] constexpr krypton::shaders::ShaderTargetType getTranspileTargetType() const override;
        void handleTranspileResult(krypton::shaders::ShaderCompileResult result) override;

    public:
        explicit Shader(Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);
        explicit Shader(Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);
        ~Shader() override = default;

        void createModule() override;
        auto getHandle() const -> VkShaderModule;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
