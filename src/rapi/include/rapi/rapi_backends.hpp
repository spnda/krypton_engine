#pragma once

#include <cstdint>

namespace krypton::rapi {
    enum class Backend : uint8_t {
        None = 0,
        Vulkan = (1 << 1),
        Metal = (1 << 2),
    };

    inline consteval Backend operator&(Backend x, Backend y) {
        return static_cast<Backend>(static_cast<uint8_t>(x) & static_cast<uint8_t>(y));
    }

    inline consteval Backend operator|(Backend x, Backend y) {
        return static_cast<Backend>(static_cast<uint8_t>(x) | static_cast<uint8_t>(y));
    }
} // namespace krypton::rapi
