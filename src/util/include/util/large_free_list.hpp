#pragma once

#include <vector>

#include <util/free_list.hpp>
#include <util/large_vector.hpp>

namespace krypton::util {
    template <typename Object, TemplateStringLiteral handleId>
    class LargeFreeList : public FreeList<Object, handleId, LargeVector<Object>> {
        // This makes the data() method inaccessible from this class.
        // We don't want to expose the data() function because a LargeVector is not backed by
        // a continuous block of memory.
        using FreeList<Object, handleId, LargeVector<Object>>::data;

    public:
        LargeFreeList() = default;
    };
} // namespace krypton::util
