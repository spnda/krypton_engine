#pragma once

#include <cmath>
#include <memory>
#include <vector>

namespace krypton::util {
    /**
     * A replacement for std::vector for very big arrays. As the cost of reallocating and moving
     * objects is very high for large std::vector's as they use geometric growth and have to copy
     * everything every time it has to reallocate, it's far more efficient to have a vector of
     * pointers to fixed size blocks that are never resized and never have to be copied again.
     *
     * By default each block is 16384 big.
     */
    template <typename T, size_t blockSize = 16384>
    class LargeVector {
        /**
         * Custom iterator to iterate over a LargeVector safely.
         */
        class Iterator {
            friend class LargeVector;

            size_t pos = 0;
            LargeVector* vector = nullptr;

            /* Private so that only our friend, LargeVector, can create a instance */
            explicit constexpr Iterator(LargeVector* vector, size_t pos);

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;

            explicit constexpr Iterator() = default;

            constexpr Iterator& operator++();         /* prefix operator */
            constexpr const Iterator operator++(int); /* postfix operator */
            constexpr T& operator*() const;
            constexpr T* operator->() const;
            constexpr bool operator==(const Iterator& other) const noexcept = default;
        };

// Apple Clang 13.1 still doesn't support std::forward_iterator...
#ifndef __APPLE__
        static_assert(std::forward_iterator<Iterator>); /* Make sure our iterator implementation is correct */
#endif

        friend class LargeVector;

        std::vector<std::unique_ptr<std::vector<T>>> data;

        constexpr void addNewBlocks(size_t count = 1) noexcept;

    public:
        /** The constructor automatically adds a first block */
        constexpr LargeVector();
        constexpr ~LargeVector() = default;

        constexpr Iterator begin() noexcept;
        [[nodiscard]] constexpr size_t capacity() const noexcept;
        constexpr Iterator end() noexcept;
        constexpr T& emplace_back(T&& value) noexcept;
        constexpr void push_back(const T& value) noexcept;
        constexpr void push_back(T&& value) noexcept;
        constexpr void resize(size_t size);
        [[nodiscard]] constexpr size_t size() const noexcept;
        [[nodiscard]] constexpr size_t size_bytes() const noexcept;

        [[nodiscard]] constexpr const T& operator[](size_t index) const;
        [[nodiscard]] constexpr T& operator[](size_t index);
    };

    template <typename T, size_t blockSize>
    constexpr LargeVector<T, blockSize>::Iterator::Iterator(LargeVector* vector, size_t pos) : pos(pos), vector(vector) {}

    template <typename T, size_t blockSize>
    constexpr auto LargeVector<T, blockSize>::Iterator::operator++() -> Iterator& {
        return (++pos, *this);
    }

    template <typename T, size_t blockSize>
    constexpr auto LargeVector<T, blockSize>::Iterator::operator++(int) -> const Iterator {
        auto& old = *this;
        ++*this;
        return old;
    }

    template <typename T, size_t blockSize>
    constexpr T& LargeVector<T, blockSize>::Iterator::operator*() const {
        return (*vector)[pos];
    }

    template <typename T, size_t blockSize>
    constexpr T* LargeVector<T, blockSize>::Iterator::operator->() const {
        return std::addressof(operator*());
    }

    template <typename T, size_t blockSize>
    constexpr LargeVector<T, blockSize>::LargeVector() {
        addNewBlocks();
    }

    template <typename T, size_t blockSize>
    constexpr void LargeVector<T, blockSize>::addNewBlocks(size_t count) noexcept {
        for (size_t i = 0; i < count; ++i) {
            data.push_back(std::move(std::make_unique<std::vector<T>>()));
            data.back()->reserve(blockSize);
        }
    }

    template <typename T, size_t blockSize>
    constexpr auto LargeVector<T, blockSize>::begin() noexcept -> Iterator {
        return Iterator { this, 0 };
    }

    template <typename T, size_t blockSize>
    constexpr size_t LargeVector<T, blockSize>::capacity() const noexcept {
        return data.capacity() * blockSize;
    }

    template <typename T, size_t blockSize>
    constexpr auto LargeVector<T, blockSize>::end() noexcept -> Iterator {
        return Iterator { this, this->size() };
    }

    template <typename T, size_t blockSize>
    constexpr T& LargeVector<T, blockSize>::emplace_back(T&& value) noexcept {
        if (data.back()->size() == blockSize)
            addNewBlocks(1);

        return data.back()->emplace_back(std::move(value));
    }

    template <typename T, size_t blockSize>
    constexpr void LargeVector<T, blockSize>::push_back(const T& value) noexcept {
        emplace_back(std::forward(value));
    }

    template <typename T, size_t blockSize>
    constexpr void LargeVector<T, blockSize>::push_back(T&& value) noexcept {
        emplace_back(std::move(value));
    }

    template <typename T, size_t blockSize>
    constexpr void LargeVector<T, blockSize>::resize(size_t size) {
        if (size > (this->size() - data.back()->size() + blockSize)) {
            /* The new requested size is larger than a single block */
            float newBlocks = static_cast<float>(size) / static_cast<float>(blockSize);
            addNewBlocks(static_cast<size_t>(std::floor(newBlocks)));
        }
    }

    template <typename T, size_t blockSize>
    constexpr size_t LargeVector<T, blockSize>::size() const noexcept {
        return (data.size() - 1) * blockSize + data.back()->size();
    }

    template <typename T, size_t blockSize>
    constexpr size_t LargeVector<T, blockSize>::size_bytes() const noexcept {
        return size() * sizeof(T);
    }

    template <typename T, size_t blockSize>
    constexpr const T& LargeVector<T, blockSize>::operator[](size_t index) const {
        return (*data[index / blockSize])[index % blockSize];
    }

    template <typename T, size_t blockSize>
    constexpr T& LargeVector<T, blockSize>::operator[](size_t index) {
        return (*data[index / blockSize])[index % blockSize];
    }
} // namespace krypton::util
