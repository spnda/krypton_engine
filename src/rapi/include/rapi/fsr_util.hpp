#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace krypton::rapi {
    struct FsrProfile {
        double factor;
        std::string name;

        FsrProfile(double factor, const char* name) noexcept : factor(factor), name(name) {};
    };

    namespace fsrProfiles {
        static FsrProfile performance = { 2.0, "Performance" };
        static FsrProfile balanced = { 1.7, "Balanced" };
        static FsrProfile quality = { 1.5, "Quality" };
        static FsrProfile ultraQuality = { 1.3, "Ultra Quality" };
    } // namespace fsrProfiles

    void configureEasu(std::vector<uint32_t>& output, glm::fvec2 renderDimensions, glm::fvec2 outputDimensions);
    [[nodiscard]] std::vector<FsrProfile> getFsrProfiles();
    [[nodiscard]] glm::ivec2 getScaledResolution(glm::ivec2 targetResolution, FsrProfile& profile);
} // namespace krypton::rapi
