#pragma once

#include <bit>
#include <limits>

namespace krypton::util {
    // The C++ stdlib does not include a constexpr or consteval version of std::pow. Therefore,
    // this is my (probably weird) constexpr implementation for std::pow for integers.
    template <typename A, typename B>
    consteval static auto consteval_pow(A a, B exp) noexcept {
        static_assert(std::numeric_limits<A>::is_integer && std::numeric_limits<B>::is_integer);
        if (std::has_single_bit(exp))
            return a << exp;
        return exp == 0 ? 1 : a * consteval_pow(a, exp - 1);
    }
} // namespace krypton::util
