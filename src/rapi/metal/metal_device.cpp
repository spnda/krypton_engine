#include <Tracy.hpp>

#include <rapi/metal/metal_buffer.hpp>
#include <rapi/metal/metal_device.hpp>
#include <rapi/metal/metal_queue.hpp>
#include <rapi/metal/metal_renderpass.hpp>
#include <rapi/metal/metal_sampler.hpp>
#include <rapi/metal/metal_shader.hpp>
#include <rapi/metal/metal_swapchain.hpp>
#include <rapi/metal/metal_sync.hpp>
#include <rapi/metal/metal_texture.hpp>
#include <shaders/shader_types.hpp>
#include <util/logging.hpp>

namespace kr = krypton::rapi;

kr::mtl::Device::Device(MTL::Device* device, krypton::rapi::DeviceFeatures features) noexcept : IDevice(features), device(device) {
    name = device->name()->cString(NS::UTF8StringEncoding);
}

std::shared_ptr<kr::IBuffer> kr::mtl::Device::createBuffer() {
    return std::make_shared<mtl::Buffer>(device);
}

std::shared_ptr<kr::IRenderPass> kr::mtl::Device::createRenderPass() {
    return std::make_shared<mtl::RenderPass>(device);
}

std::shared_ptr<kr::ISampler> kr::mtl::Device::createSampler() {
    return std::make_shared<kr::mtl::Sampler>(device);
}

std::shared_ptr<kr::ISemaphore> kr::mtl::Device::createSemaphore() {
    return std::make_shared<kr::mtl::Semaphore>(device);
}

std::shared_ptr<kr::IShader> kr::mtl::Device::createShaderFunction(std::span<const std::byte> bytes,
                                                                   krypton::shaders::ShaderSourceType type,
                                                                   krypton::shaders::ShaderStage stage) {
    ZoneScoped;
    switch (stage) {
        case krypton::shaders::ShaderStage::Fragment: {
            return std::make_shared<mtl::FragmentShader>(device, bytes, type);
        }
        case krypton::shaders::ShaderStage::Vertex: {
            return std::make_shared<mtl::VertexShader>(device, bytes, type);
        }
        default:
            krypton::log::throwError("No support for given shader stage: {}", static_cast<uint32_t>(stage));
    }
}

std::shared_ptr<kr::IShaderParameter> kr::mtl::Device::createShaderParameter() {
    ZoneScoped;
    return std::make_shared<mtl::ShaderParameter>(device);
}

std::shared_ptr<kr::ISwapchain> kr::mtl::Device::createSwapchain(Window* window) {
    ZoneScoped;
    return std::make_shared<mtl::Swapchain>(device, window);
}

std::shared_ptr<kr::ITexture> kr::mtl::Device::createTexture(rapi::TextureUsage usage) {
    ZoneScoped;
    return std::make_shared<mtl::Texture>(device, usage);
}

std::string_view kr::mtl::Device::getDeviceName() {
    return name;
}

std::shared_ptr<kr::IQueue> kr::mtl::Device::getPresentationQueue() {
    return std::make_shared<kr::mtl::Queue>(device, device->newCommandQueue());
}
