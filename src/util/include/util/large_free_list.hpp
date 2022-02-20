#pragma once

#include <vector>

#include <util/free_list.hpp>
#include <util/large_vector.hpp>

namespace krypton::util {
    template <typename Object>
    class LargeFreeList : public FreeList<Object, LargeVector> {
        // This makes the data() method inaccessible from this class.
        using FreeList<Object, LargeVector>::data;

    public:
        LargeFreeList() = default;
    };
} // namespace krypton::util
