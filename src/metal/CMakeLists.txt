add_library(metal INTERFACE)

find_library(METAL_LIBRARY Metal)
find_library(QUARTZCORE_LIBRARY QuartzCore)

if (NOT METAL_LIBRARY)
    message(FATAL_ERROR "Could not find Metal framework")
endif()
if (NOT QUARTZCORE_LIBRARY)
    message(FATAL_ERROR "Could not find QuartzCore framework")
endif()

target_link_libraries(metal INTERFACE ${METAL_LIBRARY})
target_link_libraries(metal INTERFACE ${QUARTZCORE_LIBRARY})

target_include_directories(metal INTERFACE ".")
