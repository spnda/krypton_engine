#pragma once

#include <cstddef>

namespace krypton::util {
    template <std::size_t N>
    struct TemplateStringLiteral {
        char chars[N];

        /* implicit */ constexpr TemplateStringLiteral(const char (&literal)[N]) { // NOLINT
            std::copy_n(literal, N, chars);
        }
    };
} // namespace krypton::util
