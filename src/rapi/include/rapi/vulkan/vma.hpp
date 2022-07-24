#pragma once

// Simple header for including VMA. We include volk.h beforehand to avoid the VMA to
// include vulkan.h and windows.h.
#define VK_NO_PROTOTYPES
// clang-format off
#include <volk.h>
#include <vk_mem_alloc.h>
// clang-format on
