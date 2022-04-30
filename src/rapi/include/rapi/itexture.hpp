#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include <rapi/color_encoding.hpp>

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

    static constexpr inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<size_t>(a) | static_cast<size_t>(b));
    }

    static constexpr inline TextureUsage operator&(TextureUsage a, TextureUsage b) {
        return static_cast<TextureUsage>(static_cast<size_t>(a) & static_cast<size_t>(b));
    }

    class ITexture {
    protected:
    public:
        virtual ~ITexture() = default;

        virtual void setColorEncoding(ColorEncoding encoding) = 0;
        virtual void setName(std::string_view name) = 0;

        /**
         * This finalizes the texture creation process by uploading the texture to the GPU. To be
         * consistent, this function returns *after* the texture has been fully uploaded, so it's
         * best to call this from a separate thread.
         */
        virtual void uploadTexture(uint32_t width, uint32_t height, std::span<std::byte> data, TextureFormat textureFormat) = 0;
    };

    static_assert(std::is_abstract_v<ITexture>);
} // namespace krypton::rapi
