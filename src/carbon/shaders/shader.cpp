#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <carbon/base/device.hpp>
#include <carbon/shaders/shader.hpp>
#include <shaders/file_includer.hpp>

#ifdef WITH_NV_AFTERMATH
#include <carbon/shaders/shader_database.hpp>
#endif // #ifdef WITH_NV_AFTERMATH

static std::map<carbon::ShaderStage, shaderc_shader_kind> shader_kinds {
    {carbon::ShaderStage::RayGeneration, shaderc_raygen_shader},
    {carbon::ShaderStage::ClosestHit, shaderc_closesthit_shader},
    {carbon::ShaderStage::RayMiss, shaderc_miss_shader},
    {carbon::ShaderStage::AnyHit, shaderc_anyhit_shader},
    {carbon::ShaderStage::Intersection, shaderc_intersection_shader},
    {carbon::ShaderStage::Callable, shaderc_callable_shader},
};

carbon::ShaderModule::ShaderModule(std::shared_ptr<carbon::Device> device, std::string name, const carbon::ShaderStage shaderStage)
    : device(std::move(device)), name(std::move(name)), shaderStage(shaderStage) {
}

void carbon::ShaderModule::createShaderModule() {
    VkShaderModuleCreateInfo moduleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shaderCompileResult.binary.size() * 4,
        .pCode = shaderCompileResult.binary.data(),
    };

    vkCreateShaderModule(*device, &moduleCreateInfo, nullptr, &shaderModule);

    device->setDebugUtilsName(shaderModule, name);
#ifdef WITH_NV_AFTERMATH
    carbon::ShaderDatabase::addShaderBinary(shaderCompileResult.binary);
    carbon::ShaderDatabase::addShaderWithDebugInfo(shaderCompileResult.debugBinary, shaderCompileResult.binary);
#endif // #ifdef WITH_NV_AFTERMATH
}

void carbon::ShaderModule::createShader(const std::string& filename) {
    auto shaderFile = krypton::shaders::readShaderFile(std::filesystem::path {filename});
    shaderCompileResult = krypton::shaders::compileGlslShader(shaderFile.filePath.filename().string(),
                                                              shaderFile.content, shader_kinds.at(shaderStage));

    createShaderModule();
}

VkPipelineShaderStageCreateInfo carbon::ShaderModule::getShaderStageCreateInfo() const {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = static_cast<VkShaderStageFlagBits>(this->shaderStage),
        .module = this->shaderModule,
        .pName = "main",
    };
}

carbon::ShaderStage carbon::ShaderModule::getShaderStage() const {
    return shaderStage;
}

VkShaderModule carbon::ShaderModule::getHandle() const {
    return shaderModule;
}
