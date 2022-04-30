#pragma once

#ifdef RAPI_WITH_METAL

#include <span>
#include <vector>

#include <Metal/MTLLibrary.hpp>
#include <robin_hood.h>

#include <rapi/ishader.hpp>
#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <rapi/rapi.hpp>
#include <util/reference_counter.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::metal {
    class CommandBuffer;

    class ShaderParameter : public IShaderParameter {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::metal::CommandBuffer;

        MTL::Device* device = nullptr;
        MTL::ArgumentEncoder* encoder = nullptr;
        MTL::Buffer* argumentBuffer = nullptr;

        std::shared_ptr<RenderAPI> rapi = nullptr;

        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<metal::Buffer>> buffers;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<metal::Texture>> textures;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<metal::Sampler>> samplers;

    public:
        explicit ShaderParameter(MTL::Device* device);

        void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void buildParameter() override;
        void destroy() override;
    };

    class FragmentShader : public IShader {
        friend class ::krypton::rapi::MetalBackend;

    private:
        std::string msl;
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::Function* function = nullptr;

        constexpr krypton::shaders::ShaderTargetType getTranspileTargetType() const override;
        void handleTranspileResult(krypton::shaders::ShaderCompileResult result) override;

    public:
        explicit FragmentShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);
        explicit FragmentShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);
        ~FragmentShader() override = default;

        void createModule() override;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
    };

    class VertexShader : public IShader {
        friend class ::krypton::rapi::MetalBackend;

    private:
        std::string msl;
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

        MTL::Device* device = nullptr;
        MTL::Library* library = nullptr;
        MTL::Function* function = nullptr;

        constexpr krypton::shaders::ShaderTargetType getTranspileTargetType() const override;
        void handleTranspileResult(krypton::shaders::ShaderCompileResult result) override;

    public:
        explicit VertexShader(MTL::Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);
        explicit VertexShader(MTL::Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);
        ~VertexShader() override = default;

        void createModule() override;
        bool isParameterObjectCompatible(IShaderParameter* parameter) override;
    };

    static_assert(!std::is_abstract_v<FragmentShader>);
    static_assert(!std::is_abstract_v<VertexShader>);
} // namespace krypton::rapi::metal

#endif
