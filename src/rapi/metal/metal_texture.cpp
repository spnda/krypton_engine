#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <util/assert.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::mtl {
    MTL::PixelFormat getPixelFormat(TextureFormat textureFormat) noexcept {
        switch (textureFormat) {
            case TextureFormat::RGBA8_UNORM:
                return MTL::PixelFormatRGBA8Unorm;
            case TextureFormat::RGBA8_SRGB:
                return MTL::PixelFormatRGBA8Unorm_sRGB;
            case TextureFormat::RGBA16_UNORM:
                return MTL::PixelFormatRGBA16Unorm;
            case TextureFormat::BGRA10_UNORM:
                return MTL::PixelFormatBGRA10_XR;
            case TextureFormat::BGRA10_SRGB:
                return MTL::PixelFormatBGRA10_XR_sRGB;
            case TextureFormat::BGR10_UNORM:
                return MTL::PixelFormatBGR10_XR;
            case TextureFormat::BGR10_SRGB:
                return MTL::PixelFormatBGR10_XR_sRGB;
            case TextureFormat::A2BGR10_UNORM:
                return MTL::PixelFormatRGB10A2Unorm;
            case TextureFormat::R8_UNORM:
                return MTL::PixelFormatR8Unorm;
            case TextureFormat::R8_SRGB:
                return MTL::PixelFormatR8Unorm_sRGB;
            case TextureFormat::D32_FLOAT:
                return MTL::PixelFormatDepth32Float;
            case TextureFormat::Invalid:
                return MTL::PixelFormatInvalid;
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

    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(getPixelFormat(format), width, height, false);
    texDesc->setUsage(MTL::TextureUsageShaderRead);
    texDesc->setStorageMode(MTL::StorageModeShared);
    texDesc->setSwizzle(swizzleChannels);

    texture = device->newTexture(texDesc);

    if (name != nullptr)
        texture->setLabel(name);
}

void kr::mtl::Texture::destroy() {
    ZoneScoped;
    texture->retain()->release();
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
