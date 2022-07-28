#include <algorithm>
#include <iostream>

#include <Tracy.hpp>
#include <imgui.h>

#include <core/imgui_console.hpp>
#include <util/assert.hpp>
#include <util/logging.hpp>

namespace krypton::core {
    void logCallback(kl::PrintType type, const std::string& str, void* userData) {
        VERIFY(userData != nullptr);
        auto* console = reinterpret_cast<ImGuiConsole*>(userData);
        console->print(str);
    }
} // namespace krypton::core

namespace kc = krypton::core;

kc::ImGuiConsole::~ImGuiConsole() {
    log::setPrintCallback(nullptr);
    log::setUserPointer(nullptr);
}

void kc::ImGuiConsole::drawWindow() {
    ZoneScoped;
    ImGui::Begin(windowName);

    // 0.0F maximises the child size to the maximum this window can provide.
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("Log window", ImVec2(0.0F, -footer_height_to_reserve), true);
    bufferMutex.lock();
    for (auto i = bufferIndex; i < (currentBufferSize + bufferIndex); ++i) {
        auto& line = buffer.at(i % currentBufferSize);
        if (line.hasFgColor) {
            ImGui::PushStyleColor(ImGuiCol_Text, line.fgColor);
        }
        ImGui::TextUnformatted(line.text.data(), line.text.data() + line.text.size());
        if (line.hasFgColor) {
            ImGui::PopStyleColor();
        }
    }
    bufferMutex.unlock();
    ImGui::EndChild();

    constexpr auto inputTextFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (ImGui::InputText("Console input", inputBuffer.data(), inputBufferSize, inputTextFlags)) {
        print(inputBuffer + "\n"); // Makes a copy.
    }

    ImGui::End();
}

void kc::ImGuiConsole::initCallbacks() {
    ZoneScoped;
    log::setUserPointer(this);
    log::setPrintCallback(logCallback);
    inputBuffer.resize(inputBufferSize);
}

void kc::ImGuiConsole::parseTerminalColor(ConsoleLine& line) {
    constexpr char escape = 0x1B;
    auto& str = line.originalText;

    // We will only parse colors found at the beginning of the line string.
    // The escape character plus '[' indicate the so called "CSI".
    if (str[0] != escape || str[1] != '[') {
        return;
    }

    // Now we'll have to check for the colors. 30-37 are predefined foreground colors. 38,
    // albeit non-standard, indicates a custom foreground color follows. 40-47 and 48 are
    // based on the same concept but instead modify the background, which we don't support.
    if (str[2] == '3') {
        if (str[3] >= '0' && str[3] <= '7') {
            // Predefined colors. TODO.
            std::array<ImVec4, 8> predefinedColors = {
                ImVec4(),                   // black
                ImVec4(204, 0, 0, 255),     // red
                ImVec4(78, 154, 6, 255),    // green
                ImVec4(196, 160, 0, 255),   // yellow
                ImVec4(114, 159, 207, 255), // blue
                ImVec4(117, 80, 123, 255),  // magenta
                ImVec4(6, 152, 154, 255),   // cyan
                ImVec4(211, 215, 207, 255), // white
            };
            line.fgColor = predefinedColors[str[5] - '0']; // Is this UB?
            line.hasFgColor = true;
            line.text = line.text.substr(6);
        } else if (str[3] == '8') {
            // This will be followed either by a single 0..255 integer used for all three
            // channels, or by 3 integers separated by semicolons describing the color.
            if (str[4] == ';' && str[6] == ';') {
                auto getNextValue = [str](uint64_t offset, uint64_t& end) {
                    char* x;
                    auto value = std::strtol(str.c_str() + offset, &x, 10);
                    end = x - str.c_str();
                    return value;
                };

                uint64_t offset = 0;
                auto r = getNextValue(7, offset);
                if (str[5] == '5') {
                    // Single 0..255 value.
                    line.fgColor = ImVec4(r, r, r, 255);
                } else if (str[5] == '2') {
                    // We expect 3 integers separated by semicolons.
                    auto g = getNextValue(offset + 1, offset);
                    auto b = getNextValue(offset + 1, offset);
                    line.fgColor = ImVec4(r, g, b, 255);
                }
                line.hasFgColor = str[5] == '5' || str[5] == '2';
                line.text = line.text.substr(offset + 1);
            }
        }
    }

    constexpr auto endEscapeOffset = sizeof("[0m");
    // fmt adds a CSI to reset the color back to the default after each line to ensure that no
    // later lines are affected by our work. Here, we'll try and detect that and if so remove
    // its range from the string_view because we don't require this reset.
    if (line.hasFgColor && str[str.size() - endEscapeOffset] == escape) {
        // There's a CSI at the end. Remove it from the string.
        line.text = line.text.substr(0, line.text.size() - endEscapeOffset);
    }
}

void kc::ImGuiConsole::print(std::string str) {
    ZoneScoped;
    // For debugging purposes we still want to print to the actual console.
    std::cout << str;

    ConsoleLine line;
    line.originalText = std::move(str);
    line.text = line.originalText;
    parseTerminalColor(line),

        bufferMutex.lock();
    buffer.at(bufferIndex) = std::move(line);

    bufferIndex = (++bufferIndex) % bufferSize;
    currentBufferSize = std::min(currentBufferSize + 1, 100U);
    bufferMutex.unlock();
}
