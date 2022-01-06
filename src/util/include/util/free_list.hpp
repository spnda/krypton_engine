#pragma once

#include <vector>

#include <util/large_vector.hpp>

namespace krypton::util {
    /**
     * A handle to some entry inside of a free list.
     */
    struct FreeListHandle {
        uint32_t index = 0;
        uint32_t generation = 0;

        explicit FreeListHandle(uint32_t index, uint32_t generation) : index(index), generation(generation) {}
    };

    template<typename Object>
    struct FreeListObject {
        uint32_t next = 0;
        uint32_t generation = 0;
        Object object;

        explicit FreeListObject() = default;
        explicit FreeListObject(Object object);
    };

    /**
     * A free list is a bulk data storage which can have
     * holes in it, to avoid expensive re-allocation. In
     * these holes we store indices to the next known object,
     * so that we know which objects are unused.
     */
    template <typename Object>
    class LargeFreeList : public LargeVector<FreeListObject<Object>> {
        [[nodiscard]] auto createSlot() -> uint32_t;

    public:
        /** We create the first 'hole' right away */
        LargeFreeList();

        [[nodiscard]] auto getFromHandle(FreeListHandle& handle) -> Object&;
        [[nodiscard]] auto getNewHandle() -> FreeListHandle;
        [[nodiscard]] bool isHandleValid(FreeListHandle& handle);
        void removeHandle(FreeListHandle& handle);
    };

    template<typename Object>
    krypton::util::FreeListObject<Object>::FreeListObject(Object object)
            : object(std::move(object)) {

    }

    template<typename Object>
    krypton::util::LargeFreeList<Object>::LargeFreeList() {
        this->push_back(FreeListObject<Object> {});
    }

    template<typename Object>
    uint32_t krypton::util::LargeFreeList<Object>::createSlot() {
        auto& lfl = *this;

        /** We check for the first free hole and use it. */
        const uint32_t slot = lfl[0].next;
        lfl[0].next = lfl[slot].next;
        if (slot) return slot;

        /** There are no holes yet, we extend the array. */
        this->resize(this->size() + 1);
        return this->size() - 1;
    }

    template<typename Object>
    Object& krypton::util::LargeFreeList<Object>::getFromHandle(FreeListHandle& handle) {
        /* Verify first that the handle is valid */
        assert(isHandleValid(handle));

        return (*this)[handle.index].object;
    }

    template<typename Object>
    krypton::util::FreeListHandle krypton::util::LargeFreeList<Object>::getNewHandle() {
        auto slot = createSlot();
        return krypton::util::FreeListHandle { slot, (*this)[slot].generation };
    }

    template<typename Object>
    bool krypton::util::LargeFreeList<Object>::isHandleValid(FreeListHandle& handle) {
        return (*this)[handle.index].generation == handle.generation;
    }

    template<typename Object>
    void krypton::util::LargeFreeList<Object>::removeHandle(krypton::util::FreeListHandle& handle) {
        assert(isHandleValid(handle));

        auto& lfl = *this;

        lfl[handle.index].next = lfl[0].next;
        lfl[0].next = handle.index;

        /**
         * We also want to up the generation index for each remove, so that handles
         * can be checked for validity.
         */
        lfl[handle.index].generation++;
    }
}
