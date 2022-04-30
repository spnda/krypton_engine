#pragma once

namespace krypton::rapi {
#ifdef _MSC_VER
#pragma warning(disable : 26812) // C26812: The enum type '' is unscoped. Prefer 'enum class' over 'enum'.
#endif

    // Easier if we don't use enum class because of the bitmask usage
    enum Backend : size_t {
        None = 0,
        Vulkan = (1 << 1),
        Metal = (1 << 2),
    };

#ifdef _MSC_VER
#pragma warning(default : 26812)
#endif

    inline consteval Backend operator&(Backend x, Backend y) {
        return static_cast<Backend>(static_cast<int>(x) & static_cast<int>(y));
    }

    inline consteval Backend operator|(Backend x, Backend y) {
        return static_cast<Backend>(static_cast<int>(x) | static_cast<int>(y));
    }
} // namespace krypton::rapi
