# carbon

carbon is a Vulkan abstraction with support for many Vulkan features,
including hardware accelerated ray tracing with `VK_KHR_accleration_structures`.
Additionally, this will detect the NVIDIA Aftermath library, if one is
provided under /external/nv-aftermath and will use it in the build to
provide crash reports.
