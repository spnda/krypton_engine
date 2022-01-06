#pragma once

#include <util/free_list.hpp>

namespace krypton::rapi {
    struct RenderObjectHandle : public krypton::util::FreeListHandle {
        explicit RenderObjectHandle(krypton::util::FreeListHandle handle)
                : krypton::util::FreeListHandle(handle.index, handle.generation) {}
    };
}
