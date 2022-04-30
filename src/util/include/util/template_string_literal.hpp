#pragma once

#include <algorithm> // We need this just for std::copy_n
#include <cstddef>

namespace krypton::util {
    template <std::size_t N>
    struct TemplateStringLiteral {
        char chars[N];

        consteval TemplateStringLiteral(const char (&literal)[N]) { // NOLINT
            std::copy_n(literal, N, chars);
        }
    };
} // namespace krypton::util
