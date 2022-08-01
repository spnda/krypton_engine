#pragma once

#include <Metal/MTLArgumentEncoder.hpp>
#include <robin_hood.h>

#include <rapi/ishaderparameter.hpp>

namespace krypton::rapi::mtl {
    class ShaderParameterPool final : public IShaderParameterPool {
        static constexpr uint32_t megabyte = 1024 * 1024;

        MTL::Device* device;
        MTL::Heap* heap = nullptr;
        NS::String* name = nullptr;

        std::vector<MTL::Buffer*> argumentBuffers;

    public:
        explicit ShaderParameterPool(MTL::Device* device) noexcept;
        ~ShaderParameterPool() override = default;

        bool allocate(ShaderParameterLayout layout, std::unique_ptr<IShaderParameter>& parameter) override;
        bool allocate(std::initializer_list<ShaderParameterLayout> layouts,
                      std::span<std::unique_ptr<IShaderParameter>> parameters) override;
        void create() override;
        void destroy() override;
        void reset() override;
        void setName(std::string_view newName) override;
    };

    class Buffer;
    class CommandBuffer;
    class Sampler;
    class Texture;

    class ShaderParameter final : public IShaderParameter {
        friend class ::krypton::rapi::mtl::CommandBuffer;

        MTL::Device* device;
        MTL::ArgumentEncoder* encoder;
        MTL::Buffer* argumentBuffer = nullptr;
        NS::String* name = nullptr;

        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Buffer>> buffers;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Texture>> textures;
        robin_hood::unordered_flat_map<uint32_t, std::shared_ptr<mtl::Sampler>> samplers;

    public:
        explicit ShaderParameter(MTL::Device* device, MTL::ArgumentEncoder* encoder, MTL::Buffer* buffer);

        void setBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) override;
        void setName(std::string_view name) override;
        void setSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) override;
        void setTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) override;
        void update() override;
    };
} // namespace krypton::rapi::mtl
