#pragma once

#include <vector>

#include <util/assert.hpp>
#include <util/handle.hpp>

namespace krypton::util {
    template <TemplateStringLiteral handleId>
    struct FreeListObject {
        typename Handle<handleId>::IndexSize next = 0;
        typename Handle<handleId>::IndexSize generation = 0;

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
        // TODO: Implement iterator to get values from container and mappings at the same time.
        class Iterator {
            size_t pos = 0;
            FreeList* list = nullptr;

            explicit Iterator(FreeList* list, size_t pos);

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = Object;
            using difference_type = std::ptrdiff_t;

            Iterator& operator++();   /* prefix operator */
            Iterator operator++(int); /* postfix operator */
            Object& operator*() const;
            Object* operator->() const;
            bool operator==(const Iterator& other) const noexcept = default;
        };

// Apple Clang 13.1 still doesn't support std::forward_iterator...
#ifndef __APPLE__
        static_assert(std::forward_iterator<Iterator>); /* Make sure our iterator implementation is correct */
#endif

        Container container;
        std::vector<FreeListObject<handleId>> mappings;

        [[nodiscard]] auto createSlot() -> typename Handle<handleId>::IndexSize;

    public:
        FreeList();

        [[nodiscard]] constexpr Iterator begin() noexcept;
        [[nodiscard]] auto capacity() const noexcept -> typename Handle<handleId>::IndexSize;
        [[nodiscard]] constexpr auto data() const noexcept -> const Object*;
        [[nodiscard]] constexpr Iterator end() noexcept;
        [[nodiscard]] auto getFromHandle(const Handle<handleId>& handle) -> Object&;
        [[nodiscard]] auto getNewHandle(std::shared_ptr<ReferenceCounter> refCounter) -> Handle<handleId>;
        [[nodiscard]] bool isHandleValid(const Handle<handleId>& handle);
        void removeHandle(Handle<handleId>& handle);
        [[nodiscard]] auto size() const noexcept -> typename Handle<handleId>::IndexSize;
    };

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    FreeList<Object, handleId, Container>::Iterator::Iterator(FreeList* list, size_t pos) : list(list), pos(pos) {}

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    auto FreeList<Object, handleId, Container>::Iterator::operator++() -> Iterator& {
        return (++pos, *this);
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    auto FreeList<Object, handleId, Container>::Iterator::operator++(int) -> Iterator {
        auto& old = *this;
        ++*this;
        return old;
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Object& FreeList<Object, handleId, Container>::Iterator::operator*() const {
        return (*list)[pos];
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Object* FreeList<Object, handleId, Container>::Iterator::operator->() const {
        return std::addressof(operator*());
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    FreeList<Object, handleId, Container>::FreeList() {
        container.push_back(Object {});
        mappings.emplace_back();
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    constexpr auto FreeList<Object, handleId, Container>::begin() noexcept -> Iterator {
        return { this, 0 };
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    typename Handle<handleId>::IndexSize FreeList<Object, handleId, Container>::capacity() const noexcept {
        return static_cast<typename Handle<handleId>::IndexSize>(container.capacity());
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    typename Handle<handleId>::IndexSize FreeList<Object, handleId, Container>::createSlot() {
        // We check for the first free hole and use it.
        const auto slot = mappings[0].next;
        mappings[0].next = mappings[slot].next;
        if (slot)
            return slot;

        // There are no available holes, we extend the array.
        auto size = container.size() + 1;
        container.resize(size);
        mappings.resize(size);
        return static_cast<typename Handle<handleId>::IndexSize>(size - 1);
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    constexpr const Object* FreeList<Object, handleId, Container>::data() const noexcept {
        return container.data();
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    constexpr auto FreeList<Object, handleId, Container>::end() noexcept -> Iterator {
        return { this, this->size() };
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Object& FreeList<Object, handleId, Container>::getFromHandle(const Handle<handleId>& handle) {
        // Verify first that the handle is valid and that it is "allowed" to access the object.
        if (!isHandleValid(handle)) {
            throw std::invalid_argument("The passed handle is invalid!");
        }

        return container[handle.getIndex()];
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    Handle<handleId> FreeList<Object, handleId, Container>::getNewHandle(std::shared_ptr<ReferenceCounter> refCounter) {
        auto slot = createSlot();
        container[slot] = {};
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
        if (!isHandleValid(handle)) {
            throw std::invalid_argument("The passed handle is invalid!");
        }

        mappings[handle.getIndex()].next = mappings[0].next;
        mappings[0].next = handle.getIndex();

        // We also want to up the generation index for each remove, so that handles
        // can be checked for validity.
        mappings[handle.getIndex()].generation++;
    }

    template <typename Object, TemplateStringLiteral handleId, typename Container>
    typename Handle<handleId>::IndexSize FreeList<Object, handleId, Container>::size() const noexcept {
        return static_cast<typename Handle<handleId>::IndexSize>(container.size());
    }
} // namespace krypton::util
