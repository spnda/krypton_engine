#include <bit>
#include <fmt/core.h>

#include <util/debug.hpp>

// I created this standalone C++ file to host the implementation for VMA because I don't want the
// whole VMA library to be recompiled every time I edit vulkan_backend.cpp, including the crazy
// amount of warnings it produces.
#define VMA_IMPLEMENTATION
#define VMA_ASSERT(expr)                        // We don't want VMA to do any assertions.
#define VMA_COUNT_BITS_SET(v) std::popcount(v); // Essentially a __popcnt() call if AVX is enabled

#if defined(KRYPTON_DEBUG)
#define VMA_DEBUG_LOG(format, ...) fmt::printf(format, __VA_ARGS__)
#endif

#include <carbon/vulkan.hpp>
