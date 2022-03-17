#pragma once

#include <vector>

#include <util/handle.hpp>

namespace krypton::util {
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
    template <typename Object, TemplateStringLiteral handleId, typename Container = std::vector<Object>>
    class FreeList {
        Container container;
        std::vector<FreeListObject> mappings;

        [[nodiscard]] auto createSlot() -> uint32_t;

    public:
        FreeList();

        [[nodiscard]] auto capacity() const noexcept -> uint32_t;
        [[nodiscard]] constexpr auto data() const noexcept -> const Object*;
        [[nodiscard]] auto getFromHandle(const Handle<handleId>& handle) -> Object&;
        [[nodiscard]] auto getNewHandle(std::shared_ptr<ReferenceCounter> refCounter) -> Handle<handleId>;
        [[nodiscard]] bool isHandleValid(const Handle<handleId>& handle);
        void removeHandle(Handle<handleId>& handle);
        [[nodiscard]] auto size() const noexcept -> uint32_t;
    };

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    FreeList<Object, handleId, Container>::FreeList() {
        container.push_back(Object {});
        mappings.emplace_back();
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    uint32_t FreeList<Object, handleId, Container>::capacity() const noexcept {
        return static_cast<uint32_t>(container.capacity());
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    uint32_t FreeList<Object, handleId, Container>::createSlot() {
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

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    constexpr const Object* FreeList<Object, handleId, Container>::data() const noexcept {
        return container.data();
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Object& FreeList<Object, handleId, Container>::getFromHandle(const Handle<handleId>& handle) {
        // Verify first that the handle is valid
        assert(isHandleValid(handle));

        return container[handle.getIndex()];
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Handle<handleId> FreeList<Object, handleId, Container>::getNewHandle(std::shared_ptr<ReferenceCounter> refCounter) {
        auto slot = createSlot();
        return Handle<handleId> { std::move(refCounter), slot, mappings[slot].generation };
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    bool FreeList<Object, handleId, Container>::isHandleValid(const Handle<handleId>& handle) {
        if (size() < handle.getIndex())
            return false;
        return mappings[handle.getIndex()].generation == handle.getGeneration();
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    void FreeList<Object, handleId, Container>::removeHandle(Handle<handleId>& handle) {
        assert(isHandleValid(handle));

        mappings[handle.getIndex()].next = mappings[0].next;
        mappings[0].next = handle.getIndex();

        // We also want to up the generation index for each remove, so that handles
        // can be checked for validity.
        mappings[handle.getIndex()].generation++;
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    uint32_t FreeList<Object, handleId, Container>::size() const noexcept {
        return static_cast<uint32_t>(container.size());
    }
} // namespace krypton::util
