#pragma once

#include <array>
#include <mutex>
#include <string>

#include <imgui.h>

namespace krypton::core {
    struct ConsoleLine {
        // The stripped content of the message.
        std::string originalText;
        std::string_view text;
        ImVec4 fgColor;
        // Used for filtering messages.
        uint32_t groupId;
        bool hasFgColor = false;
    };

    class ImGuiConsole final {
        static constexpr const char* const windowName = "Console";

        static constexpr std::size_t bufferSize = 100;
        std::array<ConsoleLine, bufferSize> buffer;
        std::mutex bufferMutex;
        uint32_t currentBufferSize = 0;
        uint32_t bufferIndex = 0;

        static constexpr uint32_t inputBufferSize = 100;
        std::string inputBuffer;

        void parseTerminalColor(ConsoleLine&);

    public:
        explicit ImGuiConsole() noexcept = default;
        ~ImGuiConsole();

        void drawWindow();
        void initCallbacks();
        void print(std::string);
    };
} // namespace krypton::core
