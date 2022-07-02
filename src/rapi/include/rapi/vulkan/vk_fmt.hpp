#pragma once

// This header should be included everywhere you use VkResult inside a fmtlib-formatted
// string. This will format the VkResult parameter to the corresponding enum name.

#include <fmt/format.h>
#include <robin_hood.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

inline robin_hood::unordered_flat_map<VkResult, std::string> vkResultStrings = {
    { VK_SUCCESS, "VK_SUCCESS" },
    { VK_NOT_READY, "VK_NOT_READY" },
    { VK_TIMEOUT, "VK_TIMEOUT" },
    { VK_EVENT_SET, "VK_EVENT_SET" },
    { VK_EVENT_RESET, "VK_EVENT_RESET" },
    { VK_INCOMPLETE, "VK_INCOMPLETE" },
    { VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY" },
    { VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY" },
    { VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED" },
    { VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST" },
    { VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED" },
    { VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT" },
    { VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT" },
    { VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT" },
    { VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER" },
    { VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS" },
    { VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED" },
    { VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL" },
    { VK_ERROR_UNKNOWN, "VK_ERROR_UNKNOWN" },
    { VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY" },
    { VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE" },
    { VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION" },
    { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" },
    { VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR" },
    { VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" },
    { VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR" },
    { VK_ERROR_OUT_OF_DATE_KHR, "VK_ERROR_OUT_OF_DATE_KHR" },
    { VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" },
    { VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT" },
    { VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV" },
    { VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
    { VK_ERROR_NOT_PERMITTED_EXT, "VK_ERROR_NOT_PERMITTED_EXT" },
    { VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
    { VK_THREAD_IDLE_KHR, "VK_THREAD_IDLE_KHR" },
    { VK_THREAD_DONE_KHR, "VK_THREAD_DONE_KHR" },
    { VK_OPERATION_DEFERRED_KHR, "VK_OPERATION_DEFERRED_KHR" },
    { VK_OPERATION_NOT_DEFERRED_KHR, "VK_OPERATION_NOT_DEFERRED_KHR" },
    { VK_PIPELINE_COMPILE_REQUIRED_EXT, "VK_PIPELINE_COMPILE_REQUIRED_EXT" },
};

template <>
struct fmt::formatter<VkResult> {
    template <typename ParseContext>
    constexpr inline auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    inline auto format(VkResult const& result, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", vkResultStrings[result]);
    }
};

template <>
struct fmt::formatter<VkExtensionProperties> {
    template <typename ParseContext>
    constexpr inline auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    inline auto format(VkExtensionProperties const& result, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", result.extensionName);
    }
};

template <>
struct fmt::formatter<VkLayerProperties> {
    template <typename ParseContext>
    constexpr inline auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    inline auto format(VkLayerProperties const& result, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", result.layerName);
    }
};
