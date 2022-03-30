#pragma once

#include <memory>
#include <string>
#include <vector>

#include <rapi/color_encoding.hpp>

#include <rapi/texture_format.hpp>
#include <util/reference_counter.hpp>

namespace carbon {
    class Buffer;
    class Texture;
} // namespace carbon

namespace krypton::rapi::vulkan {
    /**
     * A texture object. We don't keep the pixel data
     * around as there's no real reason to do so.
     */
    struct Texture final {
        std::string name;
        rapi::ColorEncoding encoding = ColorEncoding::LINEAR;
        rapi::TextureFormat format = TextureFormat::RGBA8;
        uint32_t width = 0, height = 0;

        // This is just a temporary storage for pixels during texture upload.
        std::vector<std::byte> pixels;

        std::unique_ptr<carbon::Texture> texture = nullptr;
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter = nullptr;

        Texture();
        Texture(Texture&& texture) noexcept;
        ~Texture();
    };
} // namespace krypton::rapi::vulkan
