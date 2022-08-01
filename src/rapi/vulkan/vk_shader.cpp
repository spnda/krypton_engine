#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_shader.hpp>
#include <shaders/shader_types.hpp>
#include <util/assert.hpp>
#include <util/bits.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

// clang-format off
constexpr std::array<std::pair<krypton::shaders::ShaderStage, VkShaderStageFlagBits>, 2> vulkanShaderStages = {{
    {krypton::shaders::ShaderStage::Fragment, VK_SHADER_STAGE_FRAGMENT_BIT},
    {krypton::shaders::ShaderStage::Vertex, VK_SHADER_STAGE_VERTEX_BIT},
}};
// clang-format on

VkShaderStageFlags kr::vk::getShaderStages(shaders::ShaderStage stages) noexcept {
    VkShaderStageFlags newStages = 0;
    for (const auto& [old_pos, new_pos] : vulkanShaderStages) {
        if (util::hasBit(stages, old_pos)) {
            newStages |= new_pos;
        }
    }
    return newStages;
}

#pragma region vk::Shader
kr::vk::Shader::Shader(Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::SPIRV) {
        needs_transpile = true;
    } else {
        spirv.resize(bytes.size() / sizeof(uint32_t));
        std::memcpy(spirv.data(), bytes.data(), bytes.size());
    }
}

kr::vk::Shader::Shader(Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::SPIRV) {
        needs_transpile = true;
    } else {
        spirv.resize(bytes.size() / sizeof(uint32_t));
        std::memcpy(spirv.data(), bytes.data(), bytes.size());
    }
}

constexpr krypton::shaders::ShaderTargetType kr::vk::Shader::getTranspileTargetType() const {
    return shaders::ShaderTargetType::SPIRV;
}

void kr::vk::Shader::handleTranspileResult(krypton::shaders::ShaderCompileResult result) {
    ZoneScoped;
    VERIFY(result.resultType == shaders::CompileResultType::SPIRV);
    VERIFY(result.resultSize % 4 == 0);

    spirv.resize(result.resultSize / sizeof(uint32_t)); // resize doesn't use bytes
    std::memcpy(spirv.data(), result.resultBytes.data(), result.resultSize);
}

void kr::vk::Shader::createModule() {
    ZoneScoped;
    VERIFY(!needs_transpile);

    VkShaderModuleCreateInfo moduleInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv.size() * sizeof(uint32_t),
        .pCode = spirv.data(),
    };
    auto res = vkCreateShaderModule(device->getHandle(), &moduleInfo, VK_NULL_HANDLE, &shader);
    if (res != VK_SUCCESS)
        kl::err("Failed to create shader module: {}", res);

    if (!name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<const uint64_t&>(shader), name.c_str());
}

void kr::vk::Shader::destroy() {
    ZoneScoped;
    vkDestroyShaderModule(device->getHandle(), shader, VK_NULL_HANDLE);
    shader = VK_NULL_HANDLE;
}

VkShaderModule kr::vk::Shader::getHandle() const {
    return shader;
}

bool kr::vk::Shader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::vk::Shader::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;

    if (shader != VK_NULL_HANDLE && !name.empty())
        device->setDebugUtilsName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<const uint64_t&>(shader), name.c_str());
}
#pragma endregion
