#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <assets/loader/ktx_loader.hpp>

namespace ka = krypton::assets;

std::vector<ka::loader::KTX::Level> ka::loader::parseKTXHeader(std::span<const std::byte> src, KTX::Header* const header) {
    if (src.size_bytes() < 80 || std::strcmp(reinterpret_cast<const char*>(src.data()), KTX::Magic) != 0) {
        // error!
    }

    // TODO: We should ensure here that KTX files are little-endian and should be treated
    //       as such.
    std::memcpy(header, src.data(), sizeof(KTX::Header));

    // Verify that the levelCount doesn't exceed the maximum
    auto maxLevels = std::log2(std::max({ header->width, header->height, header->depth }));
    if (header->levelCount > maxLevels) {
        // error!
    }

    // Ensure that the souce buffer includes enough bytes for the levels to be read from.
    if (src.size_bytes() - sizeof(KTX::Header) < sizeof(KTX::Level) * header->levelCount) {
        // error!
    }

    // Now, copy the level data from right after the header.
    std::vector<KTX::Level> out(header->levelCount);
    std::memcpy(out.data(), src.data() + sizeof(KTX::Header), sizeof(KTX::Level) * header->levelCount);
    return out;
}

std::map<std::string, std::vector<std::byte>> ka::loader::parseKTXKeyValues(std::span<std::byte> src) {
    return {};
}

std::span<const std::byte> ka::loader::getMipmapTexture(std::span<const std::byte> src, const KTX::Header* header,
                                                        const KTX::Level* level) {
    if (level == nullptr) {
        return {};
    }

    if (header->superCompressionScheme == KTX::SuperCompressionScheme::None) {
        if (level->uncompressedLength != level->length) {
            // this shouldn't happen
            return {};
        }
        return { src.data() + level->offset, level->uncompressedLength };
    }

    return {};
}
