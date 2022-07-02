#if defined(__APPLE__)
#define VK_USE_PLATFORM_METAL_EXT
#elif defined(WIN32)
// volk doesn't use vulkan.h and therefore doesn't include windows.h.
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include <volk.h>
