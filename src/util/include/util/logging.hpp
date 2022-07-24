#pragma once

#include <ctime>
#include <string>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

// We require at least fmt 9.0.0
static_assert(FMT_VERSION >= 90000);

namespace krypton::log {
    // TODO: There's a possibility that any input string might include curly braces, which breaks
    //       fmt. Probably best to sanitize/escape those characters.

    // ↓ -------------------  NO PARAMETERS  ------------------- ↓
    inline void log(const std::string& input) {
        auto t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), input);
    }

    inline void warn(const std::string& input) {
        auto t = std::time(nullptr);
        fmt::print(stdout, fmt::fg(fmt::color::orange), "{:%H:%M:%S} | {}\n", fmt::localtime(t), input);
    }

    inline void err(const std::string& input) {
        auto t = std::time(nullptr);
        fmt::print(stderr, fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}\n", fmt::localtime(t), input);
    }

    [[noreturn]] inline void throwError(const std::string& input) noexcept(false) {
        auto t = std::time(nullptr);
        fmt::print(stderr, fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}", fmt::localtime(t), input);
        throw std::runtime_error(input);
    }
    // ↑ -------------------  NO PARAMETERS  ------------------- ↑

    // ↓ ------------------- WITH PARAMETERS ------------------- ↓
    template <typename... T>
    inline void log(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), static_cast<T&&>(args)...);
        auto t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    template <typename... T>
    inline void warn(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), static_cast<T&&>(args)...);
        auto t = std::time(nullptr);
        fmt::print(stdout, fmt::fg(fmt::color::orange), "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    template <typename... T>
    inline void err(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), static_cast<T&&>(args)...);
        auto t = std::time(nullptr);
        fmt::print(stderr, fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    template <typename... T>
    [[noreturn]] inline void throwError(const std::string& input, T&&... args) noexcept(false) {
        auto f = fmt::format(fmt::runtime(input), static_cast<T&&>(args)...);
        auto t = std::time(nullptr);
        fmt::print(stderr, fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}", fmt::localtime(t), f);
        throw std::runtime_error(f);
    }
    // ↑ ------------------- WITH PARAMETERS ------------------- ↑
} // namespace krypton::log

namespace kl = krypton::log;
