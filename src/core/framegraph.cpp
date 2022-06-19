#include <rapi/framegraph/frame_graph.hpp>

namespace std {
    template <>
    struct hash<krypton::rapi::FrameGraphAttachment> {
        auto operator()(const krypton::rapi::FrameGraphAttachment& k) const {
            return std::hash<decltype(k.attachment.getIndex())>()(k.attachment.getIndex());
        }
    };
} // namespace std
