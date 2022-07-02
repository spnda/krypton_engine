#pragma once

#include <rapi/irenderpass.hpp>

namespace krypton::rapi::vk {
    class RenderPass : public IRenderPass {
        class Device* device;

        // TODO: Make pipelines their own object.
        VkPipeline pipeline = nullptr;

    public:
        explicit RenderPass(Device* device);

        void addAttachment(uint32_t index, RenderPassAttachment attachment) override;
        void build() override;
        void destroy() override;
        void setFragmentFunction(const IShader* shader) override;
        void setVertexDescriptor(VertexDescriptor descriptor) override;
        void setVertexFunction(const IShader* shader) override;
    };
} // namespace krypton::rapi::vk
