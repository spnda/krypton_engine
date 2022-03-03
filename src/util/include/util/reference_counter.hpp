#pragma once

#include <atomic>
#include <cstdint>

namespace krypton::util {
    /**
     * @brief A Counter used for reference counting.
     *
     * This reference counter is thread-safe itself. The counter
     * is wrapped within an std::atomic to ensure atomic counting.
     */
    class ReferenceCounter final {
        std::atomic<uint32_t> refCount = 0;

    public:
        constexpr ReferenceCounter() = default;

        void increment() noexcept;
        void decrement() noexcept;
        [[nodiscard]] uint32_t count() const noexcept;

        ReferenceCounter& operator++() noexcept;
        ReferenceCounter& operator--() noexcept;
    };
} // namespace krypton::util
