#include <bit>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <rapi/vulkan/vk_fmt.hpp>

// I created this standalone C++ file to host the implementation for VMA because I don't want the
// whole VMA library to be recompiled every time I edit vulkan_backend.cpp, including the crazy
// amount of warnings it produces.
#define VMA_IMPLEMENTATION
#define VMA_ASSERT(expr)                        // We don't want VMA to do any assertions.
#define VMA_COUNT_BITS_SET(v) std::popcount(v); // Essentially a __popcnt() call if AVX is enabled

#if defined(KRYPTON_DEBUG)
template <typename... Args>
inline void vmaDebugPrint(const char* format, Args&&... args) {
    fmt::printf(format, std::forward<Args>(args)...);
    fmt::printf("\n");
}

// Some weird macro shenanigans
#define VMA_DEBUG_LOG(...) ::vmaDebugPrint(__VA_ARGS__)
#endif

#include <rapi/vulkan/vma.hpp>
