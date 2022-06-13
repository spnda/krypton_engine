#pragma once

#ifdef RAPI_WITH_METAL

#include <Metal/MTLSampler.hpp>

#include <rapi/isampler.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::metal {
    class CommandBuffer;
    class ShaderParameter;

    class Sampler : public ISampler {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::metal::CommandBuffer;
        friend class ::krypton::rapi::metal::ShaderParameter;

        MTL::Device* device;
        MTL::SamplerState* samplerState;
        MTL::SamplerDescriptor* descriptor;

    public:
        explicit Sampler(MTL::Device* device);
        ~Sampler() override = default;

        void createSampler() override;
        void setAddressModeU(SamplerAddressMode mode) override;
        void setAddressModeV(SamplerAddressMode mode) override;
        void setAddressModeW(SamplerAddressMode mode) override;
    };
} // namespace krypton::rapi::metal

#endif
