#pragma once

#include <ctime>
#include <functional>
#include <string>
#include <utility>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

#include <util/attributes.hpp>

// We require at least fmt 9.0.0
static_assert(FMT_VERSION >= 90000);

namespace krypton::log {
    // TODO: There's a possibility that any input string might include curly braces, which breaks
    //       fmt. Probably best to sanitize/escape those characters.

    enum class PrintType : uint8_t {
        LOG,
        WARN,
        ERR,
    };

    // TODO: Is this really the best way to capture/redirect logging output?
    using printCallback = std::function<void(PrintType, const std::string&, void*)>;
    void defaultCallback(PrintType, const std::string&, void*);
    void setPrintCallback(printCallback);
    void setUserPointer(void*);

    // ↓ -------------------  NO PARAMETERS  ------------------- ↓
    void log(const std::string& input);
    void warn(const std::string& input);
    void err(const std::string& input);
    [[noreturn]] void throwError(const std::string& input) noexcept(false);
    // ↑ -------------------  NO PARAMETERS  ------------------- ↑

    // ↓ ------------------- WITH PARAMETERS ------------------- ↓
    template <typename... T>
    ALWAYS_INLINE inline void log(const std::string& input, T&&... args) {
        log(fmt::format(fmt::runtime(input), static_cast<T&&>(args)...));
    }

    template <typename... T>
    ALWAYS_INLINE inline void warn(const std::string& input, T&&... args) {
        warn(fmt::format(fmt::runtime(input), static_cast<T&&>(args)...));
    }

    template <typename... T>
    ALWAYS_INLINE inline void err(const std::string& input, T&&... args) {
        err(fmt::format(fmt::runtime(input), static_cast<T&&>(args)...));
    }

    template <typename... T>
    ALWAYS_INLINE [[noreturn]] inline void throwError(const std::string& input, T&&... args) noexcept(false) {
        throwError(fmt::format(fmt::runtime(input), static_cast<T&&>(args)...));
    }
    // ↑ ------------------- WITH PARAMETERS ------------------- ↑
} // namespace krypton::log

namespace kl = krypton::log;
