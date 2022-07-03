#include <array>

#include <Metal/MTLDevice.hpp>
#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
static constexpr std::array<MTL::SamplerAddressMode, 4> metalAddressModes = {
    MTL::SamplerAddressModeRepeat,              // SamplerAddressMode::Repeat = 0
    MTL::SamplerAddressModeMirrorRepeat,        // SamplerAddressMode::MirroredRepeat = 1,
    MTL::SamplerAddressModeClampToEdge,         // SamplerAddressMode::ClampToEdge = 2,
    MTL::SamplerAddressModeClampToBorderColor,  // SamplerAddressMode::ClampToBorder = 3,
};
// clang-format on

kr::mtl::Sampler::Sampler(MTL::Device* device) : device(device), samplerState(nullptr) {
    descriptor = MTL::SamplerDescriptor::alloc()->init();
}

void kr::mtl::Sampler::createSampler() {
    ZoneScoped;
    descriptor->setSupportArgumentBuffers(true);
    samplerState = device->newSamplerState(descriptor);
    descriptor->release();
}

void kr::mtl::Sampler::setAddressModeU(SamplerAddressMode mode) {
    descriptor->setSAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}

void kr::mtl::Sampler::setAddressModeV(SamplerAddressMode mode) {
    descriptor->setTAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}

void kr::mtl::Sampler::setAddressModeW(SamplerAddressMode mode) {
    descriptor->setRAddressMode(metalAddressModes[static_cast<uint16_t>(mode)]);
}

void kr::mtl::Sampler::setFilters(SamplerFilter min, SamplerFilter max) {
    descriptor->setMinFilter(static_cast<MTL::SamplerMinMagFilter>(min));
    descriptor->setMagFilter(static_cast<MTL::SamplerMinMagFilter>(max));
}

void kr::mtl::Sampler::setName(std::string_view newName) {
    // For some strange reason MTLSamplerState's label property is readonly. So this therefore only
    // works before creating the state.
    // See https://developer.apple.com/documentation/metal/mtlsamplerstate/1516329-label?language=objc
    ZoneScoped;
    if (samplerState == nullptr) {
        // We free the descriptor when building the sampler.
        name = getUTF8String(newName.data());
        descriptor->setLabel(name);
    } else {
#ifdef KRYPTON_DEBUG
        krypton::log::warn("Invalid Usage: A Metal sampler cannot be named after it has been created.");
#endif
    }
}
