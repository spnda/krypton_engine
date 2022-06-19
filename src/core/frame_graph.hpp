#pragma once

#include <optional>
#include <vector>

#include <robin_hood.h>

#include <util/handle.hpp>

namespace krypton::rapi {
    enum class FrameGraphLoadAction {
        DontCare,
        Load,
        Clear,
    };

    enum class FrameGraphStoreAction {
        DontCare,
        Store,
        Multisample,
    };

    struct FrameGraphAttachment {
        util::Handle<"Texture"> attachment;
        FrameGraphLoadAction loadAction;
        FrameGraphStoreAction storeAction;
    };

    // The swap-chain is not a texture we allocate from the RAPI. We therefore create this custom
    // structure defining if and how the swap-chain should be used as a color attachment in a
    // render pass.
    struct FrameGraphSwapChainAttachment {
        uint32_t attachmentIndex;
        FrameGraphLoadAction loadAction;
        FrameGraphStoreAction storeAction;
    };

    // Represents a single node in a frame graph. This will usually be render passes or compute
    // operations. To keep the amount of OOP to a minimum, we use this single class to represent
    // all the different types of operations. Currently, we only care about render passes, which
    // is why this node class directly represents a generic render pass.
    struct FrameGraphRenderPass {
        friend struct FrameGraph;

        std::optional<FrameGraphSwapChainAttachment> swapChainAttachment;
        // The index within this vector determines the attachment index within the backends render
        // pass. If the swapChainAttachment has a value, the colorAttachment at the index in this
        // will be ignored.
        robin_hood::unordered_flat_map<uint32_t, FrameGraphAttachment> colorAttachments;
        FrameGraphAttachment depthStencil;
    };

    // This represents a frame graph that describes all rendering operations across a single frame.
    struct FrameGraph final {
        std::vector<FrameGraphRenderPass> renderPasses;
    };
} // namespace krypton::rapi
