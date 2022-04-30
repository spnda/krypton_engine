#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <rapi/isampler.hpp>
#include <rapi/itexture.hpp>
#include <shaders/shaders.hpp>
#include <util/handle.hpp>

namespace krypton::rapi {
    class IBuffer;

    /**
     * An interface to allow a cross-platform way to create parameter objects for shaders.
     * A single parameter might include multiple objects; it acts more like a struct with members
     * than an actual single value parameter.
     *
     * In Vulkan terminology, this would be known as a VkDescriptorSet.
     */
    class IShaderParameter : public std::enable_shared_from_this<IShaderParameter> {
    protected:
        explicit IShaderParameter() = default;

        virtual ~IShaderParameter() = default;

    public:
        virtual void addBuffer(uint32_t index, std::shared_ptr<rapi::IBuffer> buffer) = 0;

        virtual void addTexture(uint32_t index, std::shared_ptr<rapi::ITexture> texture) = 0;

        virtual void addSampler(uint32_t index, std::shared_ptr<rapi::ISampler> sampler) = 0;

        virtual void buildParameter() = 0;

        virtual void destroy() = 0;
    };

    /**
     * The IShader interface is intended to be used as a base class/interface with everything
     * related to shader compiling, shader management, and shader parameters/uniforms. This
     * interface also stores a owning copy of the raw bytes of the shader input, which is usually
     * SPIR-V but can be in many other formats, depending on build and platform.
     */
    class IShader : public std::enable_shared_from_this<IShader> {
    protected:
        // This is our owning copy of bytes. Ideally, this should never reallocate and should
        // always point to the same underlying data, as we use std::span to use these bytes with
        // another type other than std::byte.
        std::vector<std::byte> bytes;
        krypton::shaders::ShaderSourceType sourceType;

        bool needs_transpile = false;
        bool has_transpiled = false;

        // The constructor creates a copy from the given bytes span. The container previously
        // holding the given bytes can be destroyed right after this constructor has completed.
        explicit IShader(std::span<const std::byte> bytes, krypton::shaders::ShaderSourceType source);

        explicit IShader(std::vector<std::byte>&& bytes, krypton::shaders::ShaderSourceType source);

        virtual ~IShader() = default;

        virtual krypton::shaders::ShaderTargetType getTranspileTargetType() const = 0;

        // This is a function for the implementation to save the transpilation result. Usually,
        // it will just be saved as a member of this object.
        virtual void handleTranspileResult(krypton::shaders::ShaderCompileResult result) = 0;

    public:
        // This actually creates the function module that can be used for creating pipelines.
        // Having not called this, this shader is effectively useless.
        virtual void createModule() = 0;

        // In debug mode, this lets the implementation check if the parameter object is compatible
        // with this shader, meaning that all shader parameters are covered, and have the correct
        // or a compatible type.
        virtual bool isParameterObjectCompatible(IShaderParameter* parameter) = 0;

        // Returns true if the data is not in a shader language or IR that is supported by the RAPI
        // directly and would first require to be transpiled at runtime for it to be usable.
        bool needsTranspile() const;

        // This uses krypton::shaders::compileShaders if [needsTranspile] returned true and creates
        // a new allocation, without invalidating the already present bytes, with the newly
        // compiled shader.
        virtual void transpile(std::string_view entryPoint, krypton::shaders::ShaderStage stage);
    };

    static_assert(std::is_abstract_v<IShader>);
} // namespace krypton::rapi
