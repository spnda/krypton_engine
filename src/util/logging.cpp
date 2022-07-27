#include <iostream>

#include <util/logging.hpp>

namespace krypton::log {
    static kl::printCallback callback = kl::defaultCallback;
    static void* userData = nullptr;
} // namespace krypton::log

void kl::defaultCallback(PrintType type, const std::string& str, [[maybe_unused]] void* user) {
    // TODO: Avoid using iostream?
    switch (type) {
        case PrintType::LOG:
        case PrintType::WARN: {
            std::cout << str;
            break;
        }
        case PrintType::ERR: {
            std::cerr << str;
            break;
        }
    }
}

void kl::setPrintCallback(kl::printCallback cb) {
    if (cb == nullptr) {
        callback = defaultCallback;
    } else {
        callback = cb;
    }
}

void kl::setUserPointer(void* pointer) {
    userData = pointer;
}

// ↓ -------------------  NO PARAMETERS  ------------------- ↓
void kl::log(const std::string& input) {
    auto time = std::time(nullptr);
    callback(PrintType::LOG, fmt::format("{:%H:%M:%S} | {}\n", fmt::localtime(time), input), userData);
}

void kl::warn(const std::string& input) {
    auto time = std::time(nullptr);
    callback(PrintType::WARN, fmt::format(fmt::fg(fmt::color::orange), "{:%H:%M:%S} | {}\n", fmt::localtime(time), input), userData);
}

void kl::err(const std::string& input) {
    auto time = std::time(nullptr);
    callback(PrintType::ERR, fmt::format(fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}\n", fmt::localtime(time), input), userData);
}

[[noreturn]] void kl::throwError(const std::string& input) noexcept(false) {
    auto time = std::time(nullptr);
    callback(PrintType::ERR, fmt::format(fmt::fg(fmt::color::red), "{:%H:%M:%S} | {}\n", fmt::localtime(time), input), userData);
    throw std::runtime_error(input);
}
// ↑ -------------------  NO PARAMETERS  ------------------- ↑
