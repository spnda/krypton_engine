#if defined(__APPLE__)
    #define VK_USE_PLATFORM_METAL_EXT
#elif defined(WIN32)
    // volk doesn't use vulkan.h and therefore doesn't include windows.h.
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

// volk.c is written in C89 and will violate some of our C++ guidelines.
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#define VOLK_IMPLEMENTATION
#include <volk.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif
