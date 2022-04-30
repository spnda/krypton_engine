#pragma once

#include <cstdlib>

namespace krypton::rapi {
    enum class TextureFormat : size_t {
        RGBA8,
        RGBA16,
        A8,
        D32,
    };

    enum class TextureUsage : size_t {
        SampledImage,
        ColorRenderTarget,
        DepthRenderTarget,
    };
} // namespace krypton::rapi
