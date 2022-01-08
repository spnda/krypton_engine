#pragma once

// This is not a header that includes all of the carbon library.
// Instead, this includes all Vulkan related headers and handles
// some pragma warnings correclty.

#pragma warning(disable : 26812) // C26812: The enum type '' is unscoped. Prefer 'enum class' over 'enum'.
#include "VkBootstrap.h"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#pragma warning(default : 4005)
