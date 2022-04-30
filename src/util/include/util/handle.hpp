#pragma once

#include <cstdint>
#include <memory>

#include <util/attributes.hpp>
#include <util/reference_counter.hpp>
#include <util/template_string_literal.hpp>

namespace krypton::util {
    /**
     * @brief A handle for some index-based list.
     *
     * A handle to index into some index-based list
     */
    template <TemplateStringLiteral id>
    class Handle {
    public:
        using IndexSize = std::size_t;

    private:
        // Our handle values
        IndexSize index = 0;
        IndexSize generation = 0;

        // Our reference counter
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

    public:
        // Creates an invalid handle. Should not be used with containers and only as a placeholder.
        explicit Handle() noexcept;

        // We require a reference counter to be passed in, as this should be created
        // by whatever is holding the resource; also being the only one interested
        // in the counter itself.
        explicit Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter);
        explicit Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter, IndexSize index, IndexSize generation);

        Handle(const Handle& other);
        Handle(Handle&& other) noexcept;

        ~Handle();

        Handle& operator=(const Handle& other);
        Handle& operator=(Handle&& other) noexcept;

        ALWAYS_INLINE [[nodiscard]] inline IndexSize getIndex() const noexcept;
        ALWAYS_INLINE [[nodiscard]] inline IndexSize getGeneration() const noexcept;
        ALWAYS_INLINE [[nodiscard]] inline bool invalid() const noexcept;

        bool operator==(util::Handle<id> handle) const noexcept;
    };

    template <TemplateStringLiteral id>
    Handle<id>::Handle() noexcept : refCounter(nullptr) {
        index = generation = std::numeric_limits<IndexSize>::max();
    }

    template <TemplateStringLiteral id>
    Handle<id>::Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter) : refCounter(refCounter) {
        refCounter->increment();
    }

    template <TemplateStringLiteral id>
    Handle<id>::Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter, IndexSize index, IndexSize generation)
        : index(index), generation(generation), refCounter(refCounter) {
        refCounter->increment();
    }

    template <TemplateStringLiteral id>
    Handle<id>::Handle(const Handle& other) : index(other.index), generation(other.generation), refCounter(other.refCounter) {
        refCounter->increment();
    }

    template <TemplateStringLiteral id>
    Handle<id>::Handle(Handle&& other) noexcept
        : index(std::move(other.index)), generation(std::move(other.generation)), refCounter(std::move(other.refCounter)) {
        other.refCounter = nullptr;
    }

    template <TemplateStringLiteral id>
    Handle<id>::~Handle() {
        // The refCounter could perhaps be moved out of the object already.
        if (refCounter) {
            refCounter->decrement();
        }
    }

    template <TemplateStringLiteral id>
    inline Handle<id>& Handle<id>::operator=(const Handle& other) {
        if (this != &other) {
            index = other.index;
            generation = other.generation;

            // We first decrement our counter, if it exists, and then copy the other counter.
            if (refCounter)
                refCounter->decrement();
            refCounter = other.refCounter;
            refCounter->increment();
        }
        return *this;
    }

    template <TemplateStringLiteral id>
    inline Handle<id>& Handle<id>::operator=(Handle&& other) noexcept {
        if (this != &other) {
            index = other.index;
            generation = other.generation;

            // We first decrement our counter, if it exists, and then move the other counter.
            if (refCounter)
                refCounter->decrement();
            refCounter = std::move(other.refCounter);
            other.refCounter = nullptr;
        }
        return *this;
    }

    template <TemplateStringLiteral id>
    inline typename Handle<id>::IndexSize Handle<id>::getIndex() const noexcept {
        return index;
    }

    template <TemplateStringLiteral id>
    inline typename Handle<id>::IndexSize Handle<id>::getGeneration() const noexcept {
        return generation;
    }

    template <TemplateStringLiteral id>
    inline bool Handle<id>::invalid() const noexcept {
        return index == std::numeric_limits<IndexSize>::max();
    }

    template <TemplateStringLiteral id>
    inline bool Handle<id>::operator==(util::Handle<id> handle) const noexcept {
        return handle.getIndex() == index && handle.getGeneration() == generation;
    }
} // namespace krypton::util
