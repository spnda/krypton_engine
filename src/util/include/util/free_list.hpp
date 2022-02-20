#pragma once

#include <vector>

namespace krypton::util {
    struct FreeListHandle {
        uint32_t index = 0;
        uint32_t generation = 0;

        explicit FreeListHandle(uint32_t index, uint32_t generation) : index(index), generation(generation) {}
    };

    struct FreeListObject {
        uint32_t next = 0;
        uint32_t generation = 0;

        explicit FreeListObject() = default;
    };

    /**
     *  A free list is a bulk data storage which can have holes
     * in it, to avoid expensive moving and copying when adding
     * or removing elements. In these holes we store indices to the
     * next known object, so that we can keep track of unused objects.
     *
     * @tparam Object The object this free list stores.
     * @tparam Container The type of container backing this list.
     *         Typically a std::vector or similar. It has to implement
     *         at least size(), capacity(), resize(), data(),
     *         push_back(T&&), and operator[int].
     */
    template <typename Object, template <typename T = Object> typename Container = std::vector>
    class FreeList {
        Container<Object> container;
        std::vector<FreeListObject> mappings;

        [[nodiscard]] auto createSlot() -> uint32_t;

    public:
        FreeList();

        [[nodiscard]] auto capacity() const noexcept -> uint32_t;
        [[nodiscard]] constexpr auto data() const noexcept -> const Object*;
        [[nodiscard]] auto getFromHandle(const FreeListHandle& handle) -> Object&;
        [[nodiscard]] auto getNewHandle() -> FreeListHandle;
        [[nodiscard]] bool isHandleValid(const FreeListHandle& handle);
        void removeHandle(FreeListHandle& handle);
        [[nodiscard]] auto size() const noexcept -> uint32_t;
    };

    template <typename Object, template <typename> typename Container>
    FreeList<Object, Container>::FreeList() {
        container.push_back(Object {});
        mappings.emplace_back();
    }

    template <typename Object, template <typename> typename Container>
    uint32_t FreeList<Object, Container>::capacity() const noexcept {
        return static_cast<uint32_t>(container.capacity());
    }

    template <typename Object, template <typename> typename Container>
    uint32_t FreeList<Object, Container>::createSlot() {
        // We check for the first free hole and use it.
        const uint32_t slot = mappings[0].next;
        mappings[0].next = mappings[slot].next;
        if (slot)
            return slot;

        // There are no available holes, we extend the array.
        auto size = container.size() + 1;
        container.resize(size);
        mappings.resize(size);
        return static_cast<uint32_t>(size - 1);
    }

    template <typename Object, template <typename> typename Container>
    constexpr const Object* FreeList<Object, Container>::data() const noexcept {
        return container.data();
    }

    template <typename Object, template <typename> typename Container>
    Object& FreeList<Object, Container>::getFromHandle(const FreeListHandle& handle) {
        // Verify first that the handle is valid
        assert(isHandleValid(handle));

        return container[handle.index];
    }

    template <typename Object, template <typename> typename Container>
    FreeListHandle FreeList<Object, Container>::getNewHandle() {
        auto slot = createSlot();
        return FreeListHandle { slot, mappings[slot].generation };
    }

    template <typename Object, template <typename> typename Container>
    bool FreeList<Object, Container>::isHandleValid(const FreeListHandle& handle) {
        if (size() < handle.index)
            return false;
        return mappings[handle.index].generation == handle.generation;
    }

    template <typename Object, template <typename> typename Container>
    void FreeList<Object, Container>::removeHandle(FreeListHandle& handle) {
        assert(isHandleValid(handle));

        mappings[handle.index].next = mappings[0].next;
        mappings[0].next = handle.index;

        // We also want to up the generation index for each remove, so that handles
        // can be checked for validity.
        mappings[handle.index].generation++;
    }

    template <typename Object, template <typename> typename Container>
    uint32_t FreeList<Object, Container>::size() const noexcept {
        return static_cast<uint32_t>(container.size());
    }
} // namespace krypton::util
