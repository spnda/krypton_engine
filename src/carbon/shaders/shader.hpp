#pragma once

#include <map>
#include <vector>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <shaders.hpp>

namespace carbon {
    enum class ShaderStage : uint64_t {
        RayGeneration = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        ClosestHit = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        RayMiss = VK_SHADER_STAGE_MISS_BIT_KHR,
        AnyHit = VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        Intersection = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
        Callable = VK_SHADER_STAGE_CALLABLE_BIT_KHR,
    };

    inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    inline bool operator>(ShaderStage a, ShaderStage b) {
        return static_cast<uint64_t>(a) > static_cast<uint64_t>(b);
    }

    // fwd.
    class Device;

    class ShaderModule {
        std::shared_ptr<carbon::Device> device;
        std::string name;

        VkShaderModule shaderModule = nullptr;
        carbon::ShaderStage shaderStage;

        krypton::shaders::ShaderCompileResult shaderCompileResult;

        void createShaderModule();

    public:
        explicit ShaderModule(std::shared_ptr<carbon::Device> device, std::string name, carbon::ShaderStage shaderStage);

        void createShader(const std::string& filename);
        [[nodiscard]] auto getShaderStageCreateInfo() const -> VkPipelineShaderStageCreateInfo;
        [[nodiscard]] auto getShaderStage() const -> carbon::ShaderStage;
        [[nodiscard]] auto getHandle() const -> VkShaderModule;
    };
} // namespace carbon
