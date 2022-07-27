#include <algorithm>
#include <iostream>

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
    log::setUserPointer(nullptr);
}

void kc::ImGuiConsole::drawWindow() {
    ImGui::Begin(windowName);

    bufferMutex.lock();
    for (auto i = bufferIndex; i < (currentBufferSize + bufferIndex); ++i) {
        ImGui::TextUnformatted(buffer.at(i % currentBufferSize).c_str());
    }
    bufferMutex.unlock();

    ImGui::End();
}

void kc::ImGuiConsole::init() {
    log::setUserPointer(this);
    log::setPrintCallback(logCallback);
}

void kc::ImGuiConsole::print(std::string str) {
    // For debugging purposes we still want to print to the actual console.
    std::cout << str;

    bufferMutex.lock();
    bufferIndex = (++bufferIndex) % bufferSize;
    currentBufferSize = std::min(currentBufferSize + 1, 100U);

    std::swap(buffer.at(bufferIndex), str);
    bufferMutex.unlock();
}
