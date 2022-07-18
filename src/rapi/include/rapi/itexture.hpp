#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include <util/nameable.hpp>

namespace krypton::rapi {
    enum class TextureFormat : uint16_t {
        Invalid = 0,

        RGBA8_UNORM,
        RGBA8_SRGB,
        RGBA16_UNORM,

        // The next are Metal-exclusive and used for 10-bit colour depth.
        BGRA10_UNORM,
        BGRA10_SRGB,
        BGR10_UNORM,
        BGR10_SRGB,

        // 10-bit color depth supported by Vulkan and Metal.
        A2BGR10_UNORM,

        // Single channel pixel formats
        R8_UNORM,
        R8_SRGB,

        // The default depth texture format.
        D32_FLOAT,
    };

    enum class TextureUsage : uint8_t {
        SampledImage = 1,
        ColorRenderTarget = 1 << 2,
        DepthRenderTarget = 1 << 3,
        TransferSource = 1 << 4,
        TransferDestination = 1 << 5,
    };

    enum class TextureSwizzle : uint8_t {
        Zero = 0,
        One = 1,
        Red = 2,
        Green = 3,
        Blue = 4,
        Alpha = 5,
    };

    struct SwizzleChannels {
        TextureSwizzle r = TextureSwizzle::Red;
        TextureSwizzle g = TextureSwizzle::Green;
        TextureSwizzle b = TextureSwizzle::Blue;
        TextureSwizzle a = TextureSwizzle::Alpha;
    };

    static constexpr inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    static constexpr inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    class ITexture : public util::Nameable {
    protected:
    public:
        ~ITexture() override = default;

        virtual void create(TextureFormat textureFormat, uint32_t width, uint32_t height) = 0;

        virtual void setSwizzling(SwizzleChannels swizzle) = 0;

        /**
         * This finalizes the texture creation process by uploading the texture to the GPU. To be
         * consistent, this function returns *after* the texture has been fully uploaded, so it's
         * best to call this from a separate thread.
         */
        virtual void uploadTexture(std::span<std::byte> data) = 0;
    };
} // namespace krypton::rapi
