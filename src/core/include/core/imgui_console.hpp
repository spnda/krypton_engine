#pragma once

#include <array>
#include <string>

namespace krypton::core {
    class ImGuiConsole final {
        static constexpr std::size_t bufferSize = 100;
        std::array<std::string, bufferSize> buffer;
        uint32_t currentBufferSize = 0;
        uint32_t bufferIndex = 0;

    public:
        explicit ImGuiConsole() noexcept = default;
    };
} // namespace krypton::core
