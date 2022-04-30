#include <Tracy.hpp>

#include <rapi/ishader.hpp>
#include <util/assert.hpp>

namespace ks = krypton::shaders;

krypton::rapi::IShader::IShader(std::span<const std::byte> bytes, ks::ShaderSourceType source)
    : bytes(bytes.begin(), bytes.end()), sourceType(source) {}

krypton::rapi::IShader::IShader(std::vector<std::byte>&& bytes, ks::ShaderSourceType source) : bytes(bytes), sourceType(source) {}

bool krypton::rapi::IShader::needsTranspile() const {
    return needs_transpile;
}

void krypton::rapi::IShader::transpile(std::string_view entryPoint, ks::ShaderStage stage) {
    ZoneScoped;
    auto input = ks::ShaderCompileInput {
        .source = { bytes.data(), bytes.size() },
        .entryPoints = { std::string { entryPoint } },
        .shaderStages = { stage },
        .sourceType = sourceType,
        .targetType = getTranspileTargetType(),
    };

    auto results = krypton::shaders::compileShaders({ input });
    VERIFY(results.size() == 1);
    handleTranspileResult(results.front());
}
