#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_device.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::metal::Device::Device(MTL::Device* device, krypton::rapi::DeviceFeatures features) noexcept : IDevice(features), device(device) {
    name = device->name()->cString(NS::UTF8StringEncoding);
}

std::shared_ptr<kr::IBuffer> kr::metal::Device::createBuffer() {
    return std::make_shared<metal::Buffer>(device);
}

std::shared_ptr<kr::IRenderPass> kr::metal::Device::createRenderPass() {
    return std::make_shared<metal::RenderPass>(device);
}

std::shared_ptr<kr::ISampler> kr::metal::Device::createSampler() {
    return std::make_shared<kr::metal::Sampler>(device);
}

std::shared_ptr<kr::IShader> kr::metal::Device::createShaderFunction(std::span<const std::byte> bytes,
                                                                     krypton::shaders::ShaderSourceType type,
                                                                     krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    switch (stage) {
        case krypton::shaders::ShaderStage::Fragment: {
            return std::make_shared<metal::FragmentShader>(device, bytes, type);
        }
        case krypton::shaders::ShaderStage::Vertex: {
            return std::make_shared<metal::VertexShader>(device, bytes, type);
        }
        default:
            krypton::log::throwError("No support for given shader stage: {}", static_cast<uint32_t>(stage));
    }
}

std::shared_ptr<kr::IShaderParameter> kr::metal::Device::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<metal::ShaderParameter>(device);
}

std::shared_ptr<kr::ITexture> kr::metal::Device::createTexture(rapi::TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<metal::Texture>(device, usage);
}

std::string_view kr::metal::Device::getDeviceName() {
    return name;
}
