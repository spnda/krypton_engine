#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

namespace krypton::assets::loader {
    // Docs for the KTX file format can be found at
    // https://github.khronos.org/KTX-Specification/ktxspec_v2.html
    namespace KTX {
        constexpr const char* const Magic = "«KTX 20»\\r\\n\\x1A\\n";

        template <typename integer>
        struct Section {
            integer offset;
            integer length;
        };

        struct Level {
            uint64_t offset;
            uint64_t length;
            uint64_t uncompressedLength;
        };

        enum class SuperCompressionScheme : uint32_t {
            None = 0,
            BasisLZ = 1,
            Zstandard = 2,
            ZLIB = 3,
        };

        struct Header {
            uint32_t vkFormat;
            uint32_t typeSize;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            uint32_t layerCount;
            uint32_t faceCount;
            uint32_t levelCount;
            SuperCompressionScheme superCompressionScheme;
            Section<uint32_t> dfd; // Data format descriptor
            Section<uint32_t> kvd; // Key value data
            Section<uint64_t> sgd; // Super compression global data
        };
    } // namespace KTX

    [[nodiscard]] std::vector<KTX::Level> parseKTXHeader(std::span<const std::byte> src, KTX::Header* header);

    [[nodiscard]] std::map<std::string, std::vector<std::byte>> parseKTXKeyValues(std::span<std::byte> src);

    [[nodiscard]] std::span<const std::byte> getMipmapTexture(std::span<const std::byte> src, const KTX::Header* header,
                                                              const KTX::Level* level);
} // namespace krypton::assets::loader
