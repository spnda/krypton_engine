#pragma once

#include <filesystem>
#include <map>
#include <vector>

#include <carbon/shaders/shader_stage.hpp>
#include <carbon/vulkan.hpp>

namespace fs = std::filesystem;

namespace carbon {
    class Device;

    inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    inline bool operator>(ShaderStage a, ShaderStage b) { return static_cast<uint64_t>(a) > static_cast<uint64_t>(b); }

    class ShaderModule {
        std::shared_ptr<carbon::Device> device;
        std::string name;

        VkShaderModule handle = nullptr;
        carbon::ShaderStage shaderStage;

        uint32_t* shaderBinary = nullptr;
        size_t shaderBinarySize = 0;
        std::vector<fs::path> includedFiles;

    public:
        explicit ShaderModule(std::shared_ptr<carbon::Device> device, std::string name, carbon::ShaderStage shaderStage);

        void createShaderModule(uint32_t* spv, size_t spvSize);
        void destroy();
        [[nodiscard]] auto getShaderStageCreateInfo() const -> VkPipelineShaderStageCreateInfo;
        [[nodiscard]] auto getShaderStage() const -> carbon::ShaderStage;
        [[nodiscard]] auto getHandle() const -> VkShaderModule;
    };
} // namespace carbon
