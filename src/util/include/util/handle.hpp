#pragma once

#include <cstdint>
#include <memory>

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
        // Our handle values
        uint32_t index = 0;
        uint32_t generation = 0;

        // Our reference counter
        std::shared_ptr<krypton::util::ReferenceCounter> refCounter;

    public:
        // We require a reference counter to be passed in, as this should be created
        // by whatever is holding the resource; also being the only one interested
        // in the counter itself.
        explicit Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter) : refCounter(refCounter) {
            refCounter->increment();
        }

        explicit Handle(const std::shared_ptr<krypton::util::ReferenceCounter>& refCounter, uint32_t index, uint32_t generation)
            : index(index), generation(generation), refCounter(refCounter) {
            refCounter->increment();
        }

        Handle(const Handle& other) : index(other.index), generation(other.generation), refCounter(other.refCounter) {
            refCounter->increment();
        }

        Handle(Handle&& other) noexcept : index(other.index), generation(other.generation), refCounter(std::move(other.refCounter)) {
            other.refCounter = nullptr;
        }

        ~Handle() {
            // The refCounter could perhaps be moved out of the object already.
            if (refCounter != nullptr)
                refCounter->decrement();
        }

        inline Handle& operator=(const Handle& other) {
            if (this != &other) {
                index = other.index;
                generation = other.generation;

                // We first decrement our counter and then copy
                // the other counter.
                refCounter->decrement();
                refCounter = other.refCounter;
                refCounter->increment();
            }
            return *this;
        }

        inline Handle& operator=(Handle&& other) noexcept {
            if (this != &other) {
                index = other.index;
                generation = other.generation;

                // We decrement our reference counter and then
                // move the other counter over.
                refCounter->decrement();
                refCounter = std::move(other.refCounter);
                other.refCounter = nullptr;
            }
            return *this;
        }

        [[nodiscard]] inline uint32_t getIndex() const noexcept {
            return index;
        }
        [[nodiscard]] inline uint32_t getGeneration() const noexcept {
            return generation;
        }
    };
} // namespace krypton::util
