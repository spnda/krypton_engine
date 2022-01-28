#ifdef WITH_NV_AFTERMATH

#include <fstream>

#include <fmt/core.h>

#include <carbon/base/crash_tracker.hpp>
#include <carbon/shaders/shader_database.hpp>

// Static callback functions.
[[maybe_unused]] void crashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onDescription(addDescription);
}

[[maybe_unused]] void gpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

/** Callback for storing shader debug infos */
[[maybe_unused]] void shaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

/** Lookup callback for shader debug info */
[[maybe_unused]] void shaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier,
                                                    PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onShaderDebugInfoLookup(identifier, setShaderDebugInfo);
}

/** Lookup callback for shaders by hashes */
[[maybe_unused]] void shaderLookupCallback(const GFSDK_Aftermath_ShaderHash* shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary,
                                           void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onShaderLookup(shaderHash, setShaderBinary);
}

/** Lookup callback for shader source with debug info */
[[maybe_unused]] void shaderSourceLookupCallback(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName,
                                                 PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData) {
    auto crashTracker = reinterpret_cast<carbon::GpuCrashTracker*>(pUserData);
    crashTracker->onShaderSourceLookup(shaderDebugName, setShaderBinary);
}

carbon::GpuCrashTracker::GpuCrashTracker() {}

void carbon::GpuCrashTracker::enable() {
    // Enable crash dumps
    checkAftermathError(GFSDK_Aftermath_EnableGpuCrashDumps(GFSDK_Aftermath_Version_API, GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
                                                            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default, gpuCrashDumpCallback,
                                                            shaderDebugInfoCallback, crashDumpDescriptionCallback, this));
}

void carbon::GpuCrashTracker::disable() const { GFSDK_Aftermath_DisableGpuCrashDumps(); }

void carbon::GpuCrashTracker::onCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize) {
    std::scoped_lock<std::mutex> lock(crashMutex);

    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    auto result = GFSDK_Aftermath_GpuCrashDump_CreateDecoder(GFSDK_Aftermath_Version_API, pGpuCrashDump, gpuCrashDumpSize, &decoder);
    if (!checkAftermathError(result, "Failed to create crash dump decoder"))
        return;

    // Query shader information
    uint32_t shaderCount = 0;
    result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount(decoder, &shaderCount);

    if (checkAftermathError(result)) {
        std::vector<GFSDK_Aftermath_GpuCrashDump_ShaderInfo> shaderInfos(shaderCount);
        result = GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo(decoder, shaderCount, shaderInfos.data());

        if (checkAftermathError(result)) {
            for (const auto& shaderInfo : shaderInfos) {
                fmt::print("Active shader: hash = {:#x}, instance = {:#x}, type = {}\n", shaderInfo.shaderHash, shaderInfo.shaderInstance,
                           static_cast<uint32_t>(shaderInfo.shaderType));
            }
        }
    }

    // Query GPU page fault information.
    GFSDK_Aftermath_GpuCrashDump_PageFaultInfo pageFaultInfo = {};
    result = GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo(decoder, &pageFaultInfo);

    if (checkAftermathError(result)) {
        // Print information about the GPU page fault.
        fmt::print(stderr, "GPU page fault at {:#x}\n", pageFaultInfo.faultingGpuVA);

        if (pageFaultInfo.bHasResourceInfo) {
            fmt::print("Fault in resource starting at {:#x}\n", pageFaultInfo.resourceInfo.gpuVa);
            fmt::print("Size of resource: ({}, {}, {}, {}) = {} bytes\n", pageFaultInfo.resourceInfo.width,
                       pageFaultInfo.resourceInfo.height, pageFaultInfo.resourceInfo.depth, pageFaultInfo.resourceInfo.mipLevels,
                       pageFaultInfo.resourceInfo.size);
            fmt::print("Format of resource: {}\n", pageFaultInfo.resourceInfo.format);
            fmt::print("Resource was destroyed: {}\n", pageFaultInfo.resourceInfo.bWasDestroyed);
        }
    }

    // Generate JSON for general crash dump information
    const uint32_t jsonDecoderFlags =
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO | GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_INFO; // Simple & quick
    uint32_t jsonSize = 0;
    result = GFSDK_Aftermath_GpuCrashDump_GenerateJSON(decoder, jsonDecoderFlags, GFSDK_Aftermath_GpuCrashDumpFormatterFlags_UTF8_OUTPUT,
                                                       shaderDebugInfoLookupCallback, shaderLookupCallback, nullptr,
                                                       shaderSourceLookupCallback, this, &jsonSize);

    if (checkAftermathError(result)) {
        std::vector<char> json(jsonSize);
        result = GFSDK_Aftermath_GpuCrashDump_GetJSON(decoder, static_cast<uint32_t>(json.size()), json.data());

        if (checkAftermathError(result)) {
            std::ofstream file("crashdump.json", std::ios::out);
            file << std::string(json.data());
            file.close();
        }
    }

    checkAftermathError(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder), "Failed to destroy crash dump decoder");
}

void carbon::GpuCrashTracker::onShaderDebugInfo(const void* shaderDebugInfo, const uint32_t shaderDebugInfoSize) {
    std::scoped_lock<std::mutex> lock(crashMutex);

    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    auto result =
        GFSDK_Aftermath_GetShaderDebugInfoIdentifier(GFSDK_Aftermath_Version_API, shaderDebugInfo, shaderDebugInfoSize, &identifier);
    if (!checkAftermathError(result, "Failed to get shader debug info"))
        return;

    // Store shader debug info in map.
    carbon::ShaderDatabase::addShaderDebugInfos(identifier, { (uint8_t*)shaderDebugInfo, shaderDebugInfoSize });
}

void carbon::GpuCrashTracker::onShaderLookup(const GFSDK_Aftermath_ShaderHash* shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) {
    carbon::ShaderDatabase::ShaderBinary shaderBinary;
    if (!carbon::ShaderDatabase::findShaderBinary(shaderHash, shaderBinary)) {
        return; // Shader not found.
    }

    setShaderBinary(shaderBinary.ptr, static_cast<uint32_t>(shaderBinary.size));
}

/**
 * Lookup for shader debug infos for given identifier.
 * Used to map SPIR-V lines to GLSL source lines.
 */
void carbon::GpuCrashTracker::onShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier,
                                                      PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) {
    carbon::ShaderDatabase::ShaderDebugInfos debugInfos;
    if (!carbon::ShaderDatabase::findShaderDebugInfos(identifier, debugInfos))
        return;
    setShaderDebugInfo(debugInfos.ptr, debugInfos.size);
}

/** Lookup callback for shader source with debug information. */
void carbon::GpuCrashTracker::onShaderSourceLookup(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName,
                                                   PFN_GFSDK_Aftermath_SetData setShaderBinary) {
    std::vector<uint32_t> shaderBinary;
    if (!carbon::ShaderDatabase::findShaderBinaryWithDebugInfo(shaderDebugName, shaderBinary)) {
        return; // Shader not found.
    }
    setShaderBinary(shaderBinary.data(), static_cast<uint32_t>(shaderBinary.size()));
}

void carbon::GpuCrashTracker::onDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription) {
    /*addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, ctx.applicationName.c_str());
    std::string version = fmt::format("v{}.{}.{}",
                                      VK_API_VERSION_MAJOR(ctx.applicationVersion),
                                      VK_API_VERSION_MINOR(ctx.applicationVersion),
                                      VK_API_VERSION_PATCH(ctx.applicationVersion));
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, version.c_str());*/
}

bool carbon::GpuCrashTracker::checkAftermathError(GFSDK_Aftermath_Result result, const std::string& message) {
    if (result != GFSDK_Aftermath_Result_Success) {
        if (!message.empty()) {
            fmt::print(stderr, "{}: {}\n", message, aftermathResultStrings.at(result));
        }
        return false;
    }
    return true;
}

#endif // #ifdef WITH_NV_AFTERMATH
