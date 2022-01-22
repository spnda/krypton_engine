#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <carbon/base/device.hpp>
#include <carbon/shaders/shader.hpp>

#ifdef WITH_NV_AFTERMATH
#include <carbon/shaders/shader_database.hpp>
#endif // #ifdef WITH_NV_AFTERMATH

carbon::ShaderModule::ShaderModule(std::shared_ptr<carbon::Device> device, std::string name, const carbon::ShaderStage shaderStage)
    : device(std::move(device)), name(std::move(name)), shaderStage(shaderStage) {}

void carbon::ShaderModule::createShaderModule(uint32_t* spv, size_t spvSize) {
    shaderBinary = spv;
    shaderBinarySize = spvSize;

    VkShaderModuleCreateInfo moduleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shaderBinarySize, /* Vulkan actually uses size_t for once */
        .pCode = shaderBinary,
    };

    vkCreateShaderModule(*device, &moduleCreateInfo, nullptr, &handle);

    device->setDebugUtilsName(handle, name);
#ifdef WITH_NV_AFTERMATH
    carbon::ShaderDatabase::addShaderBinary({ shaderBinary, shaderBinarySize });
    // carbon::ShaderDatabase::addShaderWithDebugInfo(shaderCompileResult.debugBinary, shaderCompileResult.binary);
#endif // #ifdef WITH_NV_AFTERMATH
}

void carbon::ShaderModule::destroy() { vkDestroyShaderModule(*device, handle, nullptr); }

VkPipelineShaderStageCreateInfo carbon::ShaderModule::getShaderStageCreateInfo() const {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = static_cast<VkShaderStageFlagBits>(this->shaderStage),
        .module = this->handle,
        .pName = "main",
    };
}

carbon::ShaderStage carbon::ShaderModule::getShaderStage() const { return shaderStage; }

VkShaderModule carbon::ShaderModule::getHandle() const { return handle; }
