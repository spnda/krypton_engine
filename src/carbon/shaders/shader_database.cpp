#ifdef WITH_NV_AFTERMATH

#include <carbon/base/crash_tracker.hpp>
#include <carbon/shaders/shader.hpp>
#include <carbon/shaders/shader_database.hpp>
#include <map>

// Required for std::less<GFSDK_Aftermath_ShaderHash>, part of the std::map implementation.
bool operator<(GFSDK_Aftermath_ShaderHash a, GFSDK_Aftermath_ShaderHash b) { return a.hash < b.hash; }

// Required for std::less<GFSDK_Aftermath_ShaderDebugName>, part of the std::map implementation.
bool operator<(GFSDK_Aftermath_ShaderDebugName a, GFSDK_Aftermath_ShaderDebugName b) { return strcmp(a.name, b.name) == 0; }

// Required for std::less<GFSDK_Aftermath_ShaderDebugInfoIdentifier>, part of the std::map implementation.
inline bool operator<(GFSDK_Aftermath_ShaderDebugInfoIdentifier a, GFSDK_Aftermath_ShaderDebugInfoIdentifier b) {
    if (a.id[0] == b.id[0])
        return a.id[1] < b.id[1];
    return a.id[0] < b.id[0];
}

std::map<GFSDK_Aftermath_ShaderHash, carbon::ShaderDatabase::ShaderBinary> shaderBinaries = {};
std::map<GFSDK_Aftermath_ShaderDebugName, std::vector<uint32_t>> shaderBinariesWithDebugInfo;
std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, carbon::ShaderDatabase::ShaderDebugInfos> shaderDebugInfos = {};

void carbon::ShaderDatabase::addShaderBinary(ShaderBinary binary) {
    const GFSDK_Aftermath_SpirvCode shader {
        binary.ptr,
        static_cast<uint32_t>(binary.size),
    };
    GFSDK_Aftermath_ShaderHash shaderHash;
    carbon::GpuCrashTracker::checkAftermathError(GFSDK_Aftermath_GetShaderHashSpirv(GFSDK_Aftermath_Version_API, &shader, &shaderHash));

    shaderBinaries[shaderHash] = binary;
}

void carbon::ShaderDatabase::addShaderDebugInfos(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, ShaderDebugInfos debugInfos) {
    shaderDebugInfos[identifier] = debugInfos;
}

void carbon::ShaderDatabase::addShaderWithDebugInfo(std::vector<uint32_t>& strippedBinary, std::vector<uint32_t>& binary) {
    GFSDK_Aftermath_ShaderDebugName debugName;
    const GFSDK_Aftermath_SpirvCode shader { binary.data(), static_cast<uint32_t>(binary.size()) };
    const GFSDK_Aftermath_SpirvCode strippedShader { strippedBinary.data(), static_cast<uint32_t>(strippedBinary.size()) };
    carbon::GpuCrashTracker::checkAftermathError(
        GFSDK_Aftermath_GetShaderDebugNameSpirv(GFSDK_Aftermath_Version_API, &shader, &strippedShader, &debugName));

    shaderBinariesWithDebugInfo[debugName].assign(binary.begin(), binary.end());
}

bool carbon::ShaderDatabase::findShaderBinary(const GFSDK_Aftermath_ShaderHash* shaderHash, ShaderBinary& binary) {
    auto shader = shaderBinaries.find(*shaderHash);
    if (shader == shaderBinaries.end())
        return false;
    binary = shader->second;
    return true;
}

bool carbon::ShaderDatabase::findShaderDebugInfos(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier,
                                                  ShaderDebugInfos& debugInfos) {
    auto dbg = shaderDebugInfos.find(*identifier);
    if (dbg == shaderDebugInfos.end())
        return false;
    debugInfos = dbg->second;
    return true;
}

bool carbon::ShaderDatabase::findShaderBinaryWithDebugInfo(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName,
                                                           std::vector<uint32_t>& binary) {
    auto shader = shaderBinariesWithDebugInfo.find(*shaderDebugName);
    if (shader == shaderBinariesWithDebugInfo.end())
        return false;
    binary = shader->second;
    return true;
}

#endif // #ifdef WITH_NV_AFTERMATH
