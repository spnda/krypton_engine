#pragma once

#ifdef RAPI_WITH_METAL

#include <Foundation/NSString.hpp>

#ifdef __APPLE__
// NSString by default is encoded as UTF-16.
#define NSSTRING(str) reinterpret_cast<const NS::String*>(__builtin___CFStringMakeConstantString(str))
#endif

// Requires char* to be encoded as UTF8.
inline NS::String* getUTF8String(const char* data) noexcept {
    return NS::String::string(reinterpret_cast<const char*>(data), NS::UTF8StringEncoding);
}

#endif
