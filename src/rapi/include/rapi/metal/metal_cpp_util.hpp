#pragma once

#ifdef RAPI_WITH_METAL

#include <Foundation/Foundation.hpp>

#ifdef __APPLE__
#define NSSTRING(str) reinterpret_cast<const NS::String*>(__builtin___CFStringMakeConstantString(str))
#endif

#endif
