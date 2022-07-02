#pragma once

#ifdef RAPI_WITH_METAL

#include <span>
#include <string_view>
#include <vector>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLTexture.hpp>

#include <rapi/color_encoding.hpp>
#include <rapi/itexture.hpp>
#include <util/reference_counter.hpp>

namespace krypton::rapi {
    class MetalBackend;
}

namespace krypton::rapi::metal {
    class CommandBuffer;
    class RenderPass;
    class ShaderParameter;

    MTL::PixelFormat getPixelFormat(TextureFormat format, ColorEncoding colorEncoding) noexcept;

    class Texture : public ITexture {
        friend class ::krypton::rapi::MetalBackend;
        friend class ::krypton::rapi::metal::CommandBuffer;
        friend class ::krypton::rapi::metal::RenderPass;
        friend class ::krypton::rapi::metal::ShaderParameter;

        MTL::Device* device = nullptr;
        MTL::Texture* texture = nullptr;

        NS::String* name = nullptr;
        uint32_t width = 0, height = 0;
        ColorEncoding encoding = ColorEncoding::LINEAR;
        TextureFormat format = TextureFormat::RGBA8;
        TextureUsage usage = TextureUsage::SampledImage;
        MTL::TextureSwizzleChannels swizzleChannels = {};

    public:
        explicit Texture(MTL::Device* device, rapi::TextureUsage usage);
        ~Texture() override = default;

        void create(TextureFormat format, uint32_t width, uint32_t height) override;
        void setColorEncoding(ColorEncoding encoding) override;
        void setName(std::string_view name) override;
        void setSwizzling(SwizzleChannels swizzle) override;
        void uploadTexture(std::span<std::byte> data) override;
    };
} // namespace krypton::rapi::metal

#endif
