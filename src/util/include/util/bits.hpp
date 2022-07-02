#pragma once

#include <concepts>
#include <type_traits>

#include <util/attributes.hpp>

namespace krypton::util {
    // clang-format off
    template<typename T>
    concept has_and_operator = requires (T a, T b) {
        (a & b);
    };

    template <typename T>
    requires has_and_operator<T> && (std::is_enum_v<T>&& std::is_same_v<std::underlying_type_t<T>, unsigned int>)
    ALWAYS_INLINE inline bool hasBit(T flags, T bit) {
        return (flags & bit) != static_cast<T>(0u);
    }
    // clang-format on
} // namespace krypton::util
