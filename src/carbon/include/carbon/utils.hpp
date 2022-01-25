#pragma once

#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <carbon/base/queue.hpp>
#include <carbon/vulkan.hpp>

static inline const std::unordered_map<VkResult, std::string> resultStrings = {
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

template <class T>
T getFromVkbResult(vkb::detail::Result<T> result) {
    if (!result) {
        fmt::print("Encountered vkb error: {}, {}\n", result.error().message(), resultStrings.at(result.vk_result()));
        throw std::runtime_error(result.error().message());
    }
    return result.value();
}

inline void checkResult(VkResult result, const std::string& message) {
    if (result != VK_SUCCESS) {
        auto error = fmt::format("{}: {}\n", message, resultStrings.at(result));
        std::cerr << error;
        throw std::runtime_error(error);
    }
}

inline void checkResult(carbon::Queue* queue, VkResult result, const std::string& message) {
    if (result != VK_SUCCESS) {
        // Get checkpoint data
        auto checkpoints = queue->getCheckpointData(10);
        if (checkpoints.empty()) {
            fmt::print("No checkpoints have been created.\n");
        }
        for (const auto& cp : checkpoints) {
            fmt::print("Checkpoint: {}\n", static_cast<const char*>(cp.pCheckpointMarker));
        }

        auto error = fmt::format("{}: {}\n", message, resultStrings.at(result));
        std::cerr << error;
        throw std::runtime_error(error);
    }
}

inline bool isFlagSet(const uint32_t val, const uint32_t flag) { return (val & flag) == flag; }
