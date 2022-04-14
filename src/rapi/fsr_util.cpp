#include <array>
#include <cmath>
#include <cstdint>

// Some macros for FSR
#define A_CPU 1

// These are the two FSR headers, which also get included by the GPU.
#include <ffx_a.h>
#include <ffx_fsr1.h>

#include <rapi/fsr_util.hpp>

namespace kr = krypton::rapi;

void kr::configureEasu(std::vector<uint32_t>& output, glm::fvec2 renderDimensions, glm::fvec2 outputDimensions) {
    std::array<AU1, 4> const0 = {};
    std::array<AU1, 4> const1 = {};
    std::array<AU1, 4> const2 = {};
    std::array<AU1, 4> const3 = {};
    std::array<AU1, 4> sample = {}; // We won't use this here, but its part of the output veector.

    // I'm not sure if inputSizeInPixels and inputViewportInPixels are actually supposed to be
    // the same values.
    FsrEasuCon(const0.data(), const1.data(), const2.data(), const3.data(), static_cast<AF1>(renderDimensions.x),
               static_cast<AF1>(renderDimensions.y), static_cast<AF1>(renderDimensions.x), static_cast<AF1>(renderDimensions.y),
               static_cast<AF1>(outputDimensions.x), static_cast<AF1>(outputDimensions.y));

    output.insert(output.end(), const0.begin(), const0.end());
    output.insert(output.end(), const1.begin(), const1.end());
    output.insert(output.end(), const2.begin(), const2.end());
    output.insert(output.end(), const3.begin(), const3.end());
    output.insert(output.end(), sample.begin(), sample.end());
}

std::vector<kr::FsrProfile> kr::getFsrProfiles() {
    // As per https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/docs/FidelityFX-FSR-Overview-Integration.pdf,
    // it is recommended to provide the profiles in descending order.
    using namespace fsrProfiles;
    static std::vector<kr::FsrProfile> profiles = { ultraQuality, quality, balanced, performance };
    return profiles;
}

glm::ivec2 kr::getScaledResolution(glm::ivec2 targetResolution, krypton::rapi::FsrProfile& profile) {
    return {
        targetResolution.x / profile.factor,
        targetResolution.y / profile.factor,
    };
}
