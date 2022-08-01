#pragma once

#include <robin_hood.h>

#include <rapi/ishader.hpp>
#include <rapi/ishaderparameter.hpp>

// fwd.
typedef VkShaderModule_T* VkShaderModule;

namespace krypton::rapi::vk {
    ALWAYS_INLINE [[nodiscard]] VkShaderStageFlags getShaderStages(shaders::ShaderStage stages) noexcept;

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
        void destroy() override;
        [[nodiscard]] auto getHandle() const -> VkShaderModule;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::vk
