#pragma once

#include <span>
#include <vector>

#include <Metal/MTLArgumentEncoder.hpp>
#include <Metal/MTLLibrary.hpp>
#include <robin_hood.h>

#include <rapi/ishader.hpp>
#include <shaders/shader_types.hpp>
#include <util/reference_counter.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class CommandBuffer;
    class RenderPass;

    ALWAYS_INLINE [[nodiscard]] MTL::RenderStages getRenderStages(shaders::ShaderStage stages) noexcept;

    class FragmentShader final : public IShader {
        friend class ::krypton::rapi::MetalBackend;

    private:
        std::string msl;
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

        NS::String* name = nullptr;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::Function* function = nullptr;

        [[nodiscard]] constexpr krypton::shaders::ShaderTargetType getTranspileTargetType() const override;
        void handleTranspileResult(krypton::shaders::ShaderCompileResult result) override;

    public:
        explicit FragmentShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);
        explicit FragmentShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);
        ~FragmentShader() override = default;

        void createModule() override;
        void destroy() override;
        auto getFunction() const -> MTL::Function*;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
        void setName(std::string_view name) override;
    };

    class VertexShader : public IShader {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::RenderPass;

    private:
        std::string msl;
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

        NS::String* name = nullptr;
        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::Function* function = nullptr;

        [[nodiscard]] constexpr krypton::shaders::ShaderTargetType getTranspileTargetType() const override;
        void handleTranspileResult(krypton::shaders::ShaderCompileResult result) override;

    public:
        explicit VertexShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);
        explicit VertexShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);
        ~VertexShader() override = default;

        void createModule() override;
        void destroy() override;
        auto getFunction() const -> MTL::Function*;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl
