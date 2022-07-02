#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include <rapi/color_encoding.hpp>
#include <util/nameable.hpp>

namespace krypton::rapi {
    enum class TextureFormat : uint16_t {
        RGBA8,
        RGBA16,
        // These two are Metal specific and used for 10-bit colour depth.
        BGRA10,
        BGR10,
        R8,
        D32,
    };

    enum class TextureUsage : uint32_t {
        SampledImage = 1,
        ColorRenderTarget = 1 << 2,
        DepthRenderTarget = 1 << 3,
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
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    static constexpr inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    class ITexture : public util::Nameable {
    protected:
    public:
        ~ITexture() override = default;

        virtual void create(TextureFormat textureFormat, uint32_t width, uint32_t height) = 0;

        virtual void setColorEncoding(ColorEncoding encoding) = 0;

        virtual void setSwizzling(SwizzleChannels swizzle) = 0;

        /**
         * This finalizes the texture creation process by uploading the texture to the GPU. To be
         * consistent, this function returns *after* the texture has been fully uploaded, so it's
         * best to call this from a separate thread.
         */
        virtual void uploadTexture(std::span<std::byte> data) = 0;
    };
} // namespace krypton::rapi
