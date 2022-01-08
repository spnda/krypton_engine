#pragma once

#include <map>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <vulkan/vulkan.h>

#include <shaders/shaders.hpp>

#include <carbon/shaders/shader_stage.hpp>

namespace carbon {
    class Device;

    inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    inline bool operator>(ShaderStage a, ShaderStage b) {
        return static_cast<uint64_t>(a) > static_cast<uint64_t>(b);
    }

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
