#pragma once

#include <chrono>
#include <iostream>
#include <string>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

namespace krypton::log {
    // TODO: There's a possibility that any input string might include curly braces, which breaks
    //       fmt. Probably best to sanitize/escape those characters.

    // ↓ -------------------  NO PARAMETERS  ------------------- ↓
    inline void log(const std::string& input) {
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), input);
    }

    inline void warn(const std::string& input) {
        auto f = fmt::format(fmt::fg(fmt::color::yellow), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    inline void err(const std::string& input) {
        auto f = fmt::format(fmt::fg(fmt::color::red), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    [[noreturn]] inline void throwError(const std::string& input) noexcept(false) {
        auto f = fmt::format(fmt::fg(fmt::color::red), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
        throw std::runtime_error(f);
    }
    // ↑ -------------------  NO PARAMETERS  ------------------- ↑

    // ↓ ------------------- WITH PARAMETERS ------------------- ↓
    template <typename... T>
    inline void log(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    template <typename... T>
    inline void warn(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::orange), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
    }

    template <typename... T>
    inline void err(const std::string& input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::red), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
    }

    template <typename... T>
    [[noreturn]] inline void throwError(const std::string& input, T&&... args) noexcept(false) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::red), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
        throw std::runtime_error(f);
    }
    // ↑ ------------------- WITH PARAMETERS ------------------- ↑
} // namespace krypton::log
