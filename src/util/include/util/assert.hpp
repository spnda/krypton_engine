#pragma once

#ifdef __APPLE__
// The asserts.hpp library does not work with Mac's stdlib because of an issue with std::regex, and
// we therefore mimic the macros from the library manually through the standard assert macro.
#include <cassert>
#define VERIFY(expr) assert(expr)
#define CHECK(expr) assert(expr)
#else
#define ASSERT_LOWERCASE
#include <asserts.hpp>
#endif
