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

static std::map<carbon::ShaderStage, krypton::shaders::ShaderStage> shader_kinds = {
    {carbon::ShaderStage::RayGeneration, krypton::shaders::ShaderStage::RayGen},
    {carbon::ShaderStage::ClosestHit, krypton::shaders::ShaderStage::ClosestHit},
    {carbon::ShaderStage::RayMiss, krypton::shaders::ShaderStage::Miss},
    {carbon::ShaderStage::AnyHit, krypton::shaders::ShaderStage::AnyHit},
    {carbon::ShaderStage::Intersection, krypton::shaders::ShaderStage::Intersect},
    {carbon::ShaderStage::Callable, krypton::shaders::ShaderStage::Callable},
};

carbon::ShaderModule::ShaderModule(std::shared_ptr<carbon::Device> device, std::string name, const carbon::ShaderStage shaderStage)
    : device(std::move(device)), name(std::move(name)), shaderStage(shaderStage) {
}

void carbon::ShaderModule::createShaderModule() {
    VkShaderModuleCreateInfo moduleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = shaderCompileResult.binary.size() * sizeof(uint32_t), /* Vulkan actually uses size_t for once */
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
                                                              shaderFile.content, shader_kinds.at(shaderStage),
                                                              krypton::shaders::TargetSpirv::SPV_1_5);

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
