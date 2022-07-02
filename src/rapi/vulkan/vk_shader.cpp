#include <Tracy.hpp>
#include <volk.h>

#include <rapi/vulkan/vk_device.hpp>
#include <rapi/vulkan/vk_fmt.hpp>
#include <rapi/vulkan/vk_shader.hpp>
#include <util/assert.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

#pragma region vk::ShaderParameter
kr::vk::ShaderParameter::ShaderParameter(Device* device) : device(device) {}

void kr::vk::ShaderParameter::addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) {}

void kr::vk::ShaderParameter::addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) {}

void kr::vk::ShaderParameter::addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) {}

void kr::vk::ShaderParameter::buildParameter() {}

void kr::vk::ShaderParameter::destroy() {}
#pragma endregion

#pragma region vk::Shader
kr::vk::Shader::Shader(Device* device, std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::SPIRV) {
        needs_transpile = true;
    }
}

kr::vk::Shader::Shader(Device* device, std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source)
    : IShader(bytes, source), device(device) {
    if (source != shaders::ShaderSourceType::SPIRV) {
        needs_transpile = true;
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
    auto res = vkCreateShaderModule(device->getHandle(), &moduleInfo, nullptr, &shader);
    if (res != VK_SUCCESS)
        kl::err("Failed to create shader module: {}", res);
}

bool kr::vk::Shader::isParameterObjectCompatible(IShaderParameter* parameter) {
    return true;
}

void kr::vk::Shader::setName(std::string_view newName) {
    ZoneScoped;
    name = newName;
}
#pragma endregion
