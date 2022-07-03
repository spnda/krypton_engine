#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <util/assert.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::mtl {
    MTL::PixelFormat getPixelFormat(TextureFormat textureFormat, ColorEncoding colorEncoding) noexcept {
        switch (textureFormat) {
            case TextureFormat::RGBA8: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatRGBA8Unorm;
                } else {
                    return MTL::PixelFormatRGBA8Unorm_sRGB;
                }
            }
            case TextureFormat::RGBA16: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatRGBA16Unorm;
                }
                // There's no RGBA16Unorm_sRGB
                break;
            }
            case TextureFormat::BGRA10: {
                if (colorEncoding == ColorEncoding::LINEAR)
                    return MTL::PixelFormatBGRA10_XR;
                return MTL::PixelFormatBGRA10_XR_sRGB;
            }
            case TextureFormat::BGR10: {
                if (colorEncoding == ColorEncoding::LINEAR)
                    return MTL::PixelFormatBGR10_XR;
                return MTL::PixelFormatBGR10_XR_sRGB;
            }
            case TextureFormat::R8: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatR8Unorm;
                }
                return MTL::PixelFormatR8Unorm_sRGB;
            }
            case TextureFormat::D32: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatDepth32Float;
                }
                break;
            }
        }

        return MTL::PixelFormatInvalid;
    }
} // namespace krypton::rapi::mtl

#pragma region mtl::Texture
kr::mtl::Texture::Texture(MTL::Device* device, rapi::TextureUsage usage) : device(device), usage(usage) {}

kr::mtl::Texture::Texture(MTL::Device* device, MTL::Texture* texture) : device(device), texture(texture) {}

void kr::mtl::Texture::create(TextureFormat newFormat, uint32_t newWidth, uint32_t newHeight) {
    ZoneScoped;
    format = newFormat;
    width = newWidth;
    height = newHeight;

    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(mtl::getPixelFormat(format, encoding), width, height, false);
    texDesc->setUsage(MTL::TextureUsageShaderRead);
    texDesc->setStorageMode(MTL::StorageModeShared);
    texDesc->setSwizzle(swizzleChannels);

    texture = device->newTexture(texDesc);

    if (name != nullptr)
        texture->setLabel(name);
}

void kr::mtl::Texture::setColorEncoding(ColorEncoding newEncoding) {
    encoding = newEncoding;
}

void kr::mtl::Texture::setName(std::string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (texture != nullptr)
        texture->setLabel(name);
}

void kr::mtl::Texture::setSwizzling(SwizzleChannels swizzle) {
    ZoneScoped;
    swizzleChannels.red = static_cast<MTL::TextureSwizzle>(swizzle.r);
    swizzleChannels.green = static_cast<MTL::TextureSwizzle>(swizzle.g);
    swizzleChannels.blue = static_cast<MTL::TextureSwizzle>(swizzle.b);
    swizzleChannels.alpha = static_cast<MTL::TextureSwizzle>(swizzle.a);
}

void kr::mtl::Texture::uploadTexture(std::span<std::byte> data) {
    ZoneScoped;
    VERIFY(texture);

    MTL::Region imageRegion = MTL::Region::Make2D(0, 0, width, height);
    texture->replaceRegion(imageRegion, 0, data.data(), data.size_bytes() / height);
}
#pragma endregion

#pragma region mtl::Drawable
kr::mtl::Drawable::Drawable(MTL::Device* device, CA::MetalDrawable* drawable) : Texture(device, drawable->texture()), drawable(drawable) {}
#pragma endregion
