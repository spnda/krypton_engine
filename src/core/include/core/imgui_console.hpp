#pragma once

#include <array>
#include <mutex>
#include <string>

namespace krypton::core {
    class ImGuiConsole final {
        constexpr static const char* const windowName = "Console";

        static constexpr std::size_t bufferSize = 100;
        std::array<std::string, bufferSize> buffer;
        std::mutex bufferMutex;
        uint32_t currentBufferSize = 0;
        uint32_t bufferIndex = 0;

    public:
        explicit ImGuiConsole() noexcept = default;
        ~ImGuiConsole();

        void drawWindow();
        void init();
        void print(std::string);
    };
} // namespace krypton::core
