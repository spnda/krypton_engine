#include <array>

#include <Metal/MTLDevice.hpp>
#include <Tracy.hpp>

#include <rapi/metal/metal_sampler.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<MTL::SamplerAddressMode, 4> metalAddressModes = {
    MTL::SamplerAddressModeRepeat,              // SamplerAddressMode::Repeat = 0
    MTL::SamplerAddressModeMirrorRepeat,        // SamplerAddressMode::MirroredRepeat = 1,
    MTL::SamplerAddressModeClampToEdge,         // SamplerAddressMode::ClampToEdge = 2,
    MTL::SamplerAddressModeClampToBorderColor,  // SamplerAddressMode::ClampToBorder = 3,
};
// clang-format on

kr::metal::Sampler::Sampler(MTL::Device* device) : device(device), samplerState(nullptr) {
    descriptor = MTL::SamplerDescriptor::alloc()->init();
}

void kr::metal::Sampler::createSampler() {
    ZoneScoped;
    descriptor->setSupportArgumentBuffers(true);
    samplerState = device->newSamplerState(descriptor);
    descriptor->release();
}

void kr::metal::Sampler::setAddressModeU(SamplerAddressMode mode) {
    descriptor->setSAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}

void kr::metal::Sampler::setAddressModeV(SamplerAddressMode mode) {
    descriptor->setTAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}

void kr::metal::Sampler::setAddressModeW(SamplerAddressMode mode) {
    descriptor->setRAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}
