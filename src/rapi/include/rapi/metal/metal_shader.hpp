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
    class Buffer;
    class CommandBuffer;
    class RenderPass;
    class Sampler;
    class Texture;

    class ShaderParameter final : public IShaderParameter {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::CommandBuffer;

        MTL::Device* device = nullptr;
        MTL::ArgumentEncoder* encoder = nullptr;
        MTL::Buffer* argumentBuffer = nullptr;
        NS::String* name = nullptr;

        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Buffer>> buffers;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Texture>> textures;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Sampler>> samplers;

    public:
        explicit ShaderParameter(MTL::Device* device);

        void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void buildParameter() override;
        void destroy() override;
        void setName(std::string_view name) override;
    };

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
        auto getFunction() const -> MTL::Function*;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl
