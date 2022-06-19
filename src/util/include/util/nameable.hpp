#pragma once

#include <string_view>

namespace krypton::util {
    struct Nameable {
        virtual ~Nameable() = default;
        virtual void setName(std::u8string_view name) = 0;
    };
} // namespace krypton::util
