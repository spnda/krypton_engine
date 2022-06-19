#include <Tracy.hpp>

#include <rapi/metal/metal_cpp_util.hpp>
#include <rapi/metal/metal_texture.hpp>

namespace kr = krypton::rapi;

namespace krypton::rapi::metal {
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
            case TextureFormat::A8: {
                if (colorEncoding == ColorEncoding::LINEAR) {
                    return MTL::PixelFormatA8Unorm;
                }
                // There's no A8Unorm_sRGB
                break;
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
} // namespace krypton::rapi::metal

kr::metal::Texture::Texture(MTL::Device* device, rapi::TextureUsage usage) : device(device), usage(usage) {}

void kr::metal::Texture::setColorEncoding(ColorEncoding newEncoding) {
    encoding = newEncoding;
}

void kr::metal::Texture::setName(std::u8string_view newName) {
    ZoneScoped;
    name = getUTF8String(newName.data());

    if (texture != nullptr)
        texture->setLabel(name);
}

void kr::metal::Texture::uploadTexture(uint32_t width, uint32_t height, std::span<std::byte> data, TextureFormat textureFormat) {
    ZoneScoped;
    auto* texDesc = MTL::TextureDescriptor::texture2DDescriptor(metal::getPixelFormat(textureFormat, encoding), width, height, false);
    texDesc->setUsage(MTL::TextureUsageShaderRead);
    texDesc->setStorageMode(MTL::StorageModeShared);

    texture = device->newTexture(texDesc);
    if (name != nullptr)
        texture->setLabel(name);

    MTL::Region imageRegion = MTL::Region::Make2D(0, 0, width, height);
    texture->replaceRegion(imageRegion, 0, data.data(), data.size_bytes() / height);
}
