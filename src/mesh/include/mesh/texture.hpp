#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace krypton::mesh {
    struct Texture final {
        std::filesystem::path filePath;
        uint32_t width = 0, height = 0;
        uint32_t mipLevels = 0;
        std::vector<std::byte> pixels = {};
    };
} // namespace krypton::mesh
