#pragma once

#include <chrono>
#include <iostream>
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/core.h>

namespace krypton::log {
    // ↓ -------------------  NO PARAMETERS  ------------------- ↓
    inline void log(std::string input) {
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), input);
    }

    inline void warn(std::string input) {
        auto f = fmt::format(fmt::fg(fmt::color::yellow), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    inline void err(std::string input) {
        auto f = fmt::format(fmt::fg(fmt::color::red), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    inline void throwError(std::string input) {
        auto f = fmt::format(fmt::fg(fmt::color::red), input);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
        throw std::runtime_error(f);
    }
    // ↑ -------------------  NO PARAMETERS  ------------------- ↑

    // ↓ ------------------- WITH PARAMETERS ------------------- ↓
    template <typename... T>
    inline void log(std::string input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        std::time_t t = std::time(nullptr);
        fmt::print(stdout, "{:%H:%M:%S} | {}\n", fmt::localtime(t), f);
    }

    template <typename... T>
    inline void warn(std::string input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::orange), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
    }

    template <typename... T>
    inline void err(std::string input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::red), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
    }

    template<typename... T>
    inline void throwError(std::string input, T&&... args) {
        auto f = fmt::format(fmt::runtime(input), args...);
        auto c = fmt::format(fmt::fg(fmt::color::red), f);
        std::time_t t = std::time(nullptr);
        fmt::print(stderr, "{:%H:%M:%S} | {}\n", fmt::localtime(t), c);
        throw std::runtime_error(f);
    }
    // ↑ ------------------- WITH PARAMETERS ------------------- ↑
} // namespace krypton::log
