#pragma once

#ifdef WITH_NV_AFTERMATH
// We only want this to exist if aftermath exists as well.

#include <map>
#include <mutex>
#include <unordered_map>

#include <carbon/vulkan.hpp>

#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"

/**
 * A map with stringified versions of every aftermath result value.
 * Does not include DirectX specific errors. */
static inline const std::unordered_map<GFSDK_Aftermath_Result, std::string> aftermathResultStrings = {
    { GFSDK_Aftermath_Result_Success, "GFSDK_Aftermath_Result_Success" },
    { GFSDK_Aftermath_Result_NotAvailable, "GFSDK_Aftermath_Result_NotAvailable" },
    { GFSDK_Aftermath_Result_Fail, "GFSDK_Aftermath_Result_Fail" },
    { GFSDK_Aftermath_Result_FAIL_VersionMismatch, "GFSDK_Aftermath_Result_FAIL_VersionMismatch" },
    { GFSDK_Aftermath_Result_FAIL_NotInitialized, "GFSDK_Aftermath_Result_FAIL_NotInitialized" },
    { GFSDK_Aftermath_Result_FAIL_InvalidAdapter, "GFSDK_Aftermath_Result_FAIL_InvalidAdapter" },
    { GFSDK_Aftermath_Result_FAIL_InvalidParameter, "GFSDK_Aftermath_Result_FAIL_InvalidParameter" },
    { GFSDK_Aftermath_Result_FAIL_Unknown, "GFSDK_Aftermath_Result_FAIL_Unknown" },
    { GFSDK_Aftermath_Result_FAIL_ApiError, "GFSDK_Aftermath_Result_FAIL_ApiError" },
    { GFSDK_Aftermath_Result_FAIL_NvApiIncompatible, "GFSDK_Aftermath_Result_FAIL_NvApiIncompatible" },
    { GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList,
      "GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList" },
    { GFSDK_Aftermath_Result_FAIL_AlreadyInitialized, "GFSDK_Aftermath_Result_FAIL_AlreadyInitialized" },
    { GFSDK_Aftermath_Result_FAIL_DriverInitFailed, "GFSDK_Aftermath_Result_FAIL_DriverInitFailed" },
    { GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported, "GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported" },
    { GFSDK_Aftermath_Result_FAIL_OutOfMemory, "GFSDK_Aftermath_Result_FAIL_OutOfMemory" },
    { GFSDK_Aftermath_Result_FAIL_GetDataOnBundle, "GFSDK_Aftermath_Result_FAIL_GetDataOnBundle" },
    { GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext, "GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext" },
    { GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled, "GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled" },
    { GFSDK_Aftermath_Result_FAIL_NoResourcesRegistered, "GFSDK_Aftermath_Result_FAIL_NoResourcesRegistered" },
    { GFSDK_Aftermath_Result_FAIL_ThisResourceNeverRegistered, "GFSDK_Aftermath_Result_FAIL_ThisResourceNeverRegistered" },
    { GFSDK_Aftermath_Result_FAIL_Disabled, "GFSDK_Aftermath_Result_FAIL_Disabled" }
};

namespace carbon {
    // Wrapper around NVIDIA Aftermath
    class GpuCrashTracker {
        mutable std::mutex crashMutex;

    public:
        explicit GpuCrashTracker();
        GpuCrashTracker(const GpuCrashTracker& c) = delete;
        GpuCrashTracker(GpuCrashTracker&& c) = delete;

        void enable();
        void disable() const;

        void onCrashDump(const void* pGpuCrashDump, uint32_t gpuCrashDumpSize);
        void onShaderDebugInfo(const void* pShaderDebugInfo, uint32_t shaderDebugInfoSize);
        void onShaderLookup(const GFSDK_Aftermath_ShaderHash* shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary);
        /** Callback function invoked to find shader debug information data. */
        void onShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* identifier,
                                     PFN_GFSDK_Aftermath_SetData setShaderDebugInfo);
        /** Callback function invoked to find shader source debug data by shader debug name. */
        void onShaderSourceLookup(const GFSDK_Aftermath_ShaderDebugName* shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary);
        void onDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);

        /**
         * Returns true if the result is success.
         * If failed and message is not empty, it will also print the message.
         */
        static bool checkAftermathError(GFSDK_Aftermath_Result result, const std::string& message = {});
    };
} // namespace carbon

#endif // #ifdef WITH_NV_AFTERMATH
