#pragma once

#include <cassert>
#include <version>

#if defined(_LIBCPP_VERSION) || defined(__APPLE__)
    // libc++ used by (Apple) Clang has an issue with std::regex and the asserts.hpp library.
    // See https://github.com/jeremy-rifkin/libassert/issues/15
    // Also, the library uses addr2line for retrieving symbol names in the stacktrace which does not
    // exist on Mac by default.
    #include <cassert>
    #ifdef VERIFY
        #undef VERIFY
    #endif
    #ifdef CHECK
        #undef CHECK
    #endif

    #define VERIFY(expr) assert(expr)
    #define CHECK(expr) assert(expr)
#else
    #ifdef assert
        #undef assert
    #endif

    #define ASSERT_LOWERCASE
    #include <asserts.hpp>
#endif
