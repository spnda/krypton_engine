#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace krypton::assets {
    struct Texture final {
        std::filesystem::path filePath;
        std::string name;
        uint32_t width = 0, height = 0, channels = 0;
        uint32_t mipLevels = 0;
        uint8_t bitsPerPixel = 8; // We should only really allow 6-bits, but that's not possible...
        std::vector<std::byte> pixels = {};
    };
} // namespace krypton::assets
