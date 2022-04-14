#pragma once

// Deliberately no namespace as otherwise we'd have to use the using namespace declaration which
// really nobobdy likes to see.

inline uint64_t operator"" _GB(unsigned long long value) {
    return value * 1024 * 1024 * 1024;
}

inline uint64_t operator"" _MB(unsigned long long value) {
    return value * 1024 * 1024;
}

inline uint64_t operator"" _KB(unsigned long long value) {
    return value * 1024;
}
