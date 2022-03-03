#include <util/reference_counter.hpp>

namespace ku = krypton::util;

void ku::ReferenceCounter::increment() noexcept {
    ++refCount;
}

void ku::ReferenceCounter::decrement() noexcept {
    --refCount;
}

uint32_t ku::ReferenceCounter::count() const noexcept {
    return refCount;
}

ku::ReferenceCounter& ku::ReferenceCounter::operator++() noexcept {
    increment();
    return *this;
}

ku::ReferenceCounter& ku::ReferenceCounter::operator--() noexcept {
    decrement();
    return *this;
}
