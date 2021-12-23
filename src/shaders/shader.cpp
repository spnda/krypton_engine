#include <fstream>
#include <sstream>
#include <string>

#include "shader.hpp"

krypton::shaders::Shader krypton::shaders::readShaderFile(std::filesystem::path path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();

    return { path, ss.str() };
}
