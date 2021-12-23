#pragma once

#include <filesystem>

namespace krypton::shaders {
    /* Represents a shader file. */
    struct Shader {
        std::filesystem::path filePath;
        std::string content;
    };

    Shader readShaderFile(std::filesystem::path path);
}
