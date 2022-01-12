#pragma once

#ifdef WITH_NV_AFTERMATH

#include <vector>

#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"

namespace carbon::ShaderDatabase {
    struct ShaderBinary {
        uint32_t* ptr;
        size_t size;
    };

    struct ShaderDebugInfos {
        uint8_t* ptr;
        uint32_t size;
    };

    void addShaderBinary(ShaderBinary shader);
    void addShaderDebugInfos(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, ShaderDebugInfos debugInfos);
    void addShaderWithDebugInfo(std::vector<uint32_t>& strippedBinary, std::vector<uint32_t>& binary);
    bool findShaderBinary(const GFSDK_Aftermath_ShaderHash* shaderHash, ShaderBinary& binary);
    bool findShaderDebugInfos(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier, ShaderDebugInfos& debugInfos);
    bool findShaderBinaryWithDebugInfo(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName, std::vector<uint32_t>& binary);
} // namespace carbon::ShaderDatabase

#endif // #ifdef WITH_NV_AFTERMATH
