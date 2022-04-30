#pragma once

#if defined(__GNUC__) || defined(__clang__)
// There's also clang::always_inline, though clang supports both.
#define ALWAYS_INLINE [[gnu::always_inline]]
#else
#define ALWAYS_INLINE
#endif
