#pragma once

#include <Metal/MTLSampler.hpp>

#include <rapi/isampler.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::mtl {
    class CommandBuffer;
    class ShaderParameter;

    class Sampler final : public ISampler {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::mtl::CommandBuffer;
        friend class ::krypton::rapi::mtl::ShaderParameter;

        MTL::Device* device;
        NS::String* name = nullptr;
        MTL::SamplerState* samplerState;
        MTL::SamplerDescriptor* descriptor;

    public:
        explicit Sampler(MTL::Device* device);
        ~Sampler() override = default;

        void createSampler() override;
        void setAddressModeU(SamplerAddressMode mode) override;
        void setAddressModeV(SamplerAddressMode mode) override;
        void setAddressModeW(SamplerAddressMode mode) override;
        void setFilters(SamplerFilter min, SamplerFilter max) override;
        void setName(std::string_view name) override;
    };
} // namespace krypton::rapi::mtl
