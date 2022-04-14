#pragma once

#include <rapi/color_encoding.hpp>
#include <rapi/texture_format.hpp>

namespace krypton::rapi::metal {
    struct Texture final {
        // This should only be non-null when the texture is finished uploading.
        MTL::Texture* texture = nullptr;

        uint32_t width = 0, height = 0;
        std::vector<std::byte> textureData;

        std::shared_ptr<util::ReferenceCounter> refCounter = nullptr;

        std::string name;
        ColorEncoding encoding = ColorEncoding::LINEAR;
        TextureFormat format = TextureFormat::RGBA8;

        explicit Texture() = default;
    };
} // namespace krypton::rapi::metal
