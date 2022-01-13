# NVIDIA Aftermath as a CMake target
if (EXISTS "${CMAKE_SOURCE_DIR}/external/nv-aftermath" AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND WIN32)
    set(NV_AFTERMATH_DIRECTORY "${CMAKE_SOURCE_DIR}/external/nv-aftermath")
    message(STATUS "Found NVIDIA Aftermath")
    
    add_library(nv-aftermath UNKNOWN IMPORTED GLOBAL)
    add_library(nv::aftermath ALIAS nv-aftermath)
    set_target_properties(nv-aftermath PROPERTIES IMPORTED_LOCATION "${NV_AFTERMATH_DIRECTORY}/lib/x64/GFSDK_Aftermath_Lib.x64.lib")
    set_property(TARGET nv-aftermath PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${NV_AFTERMATH_DIRECTORY}/include")
    option(WITH_NV_AFTERMATH "Use NVIDIA Aftermath" ON)

    # Move the Aftermath DLL into the binary folder.
    add_custom_target(nv-aftermath-copy ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NV_AFTERMATH_DIRECTORY}/lib/x64/GFSDK_Aftermath_Lib.x64.dll"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
endif()

# slang doesn't provide builds for MacOS.
if (EXISTS "${CMAKE_SOURCE_DIR}/external/slang/bin" AND NOT APPLE)
    message(STATUS "Found slang")

    # slang
    add_library(slang-core UNKNOWN IMPORTED GLOBAL)
    add_library(slang::core ALIAS slang-core)
    set_property(TARGET slang-core PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/slang")
    if (WIN32)
        set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.lib")
        
        add_custom_target(slang-core-copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang.dll")
        add_dependencies(slang-core slang-core-copy)
    elseif (UNIX)
        set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so")
        
        add_custom_target(slang-core-copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libslang.so")
            add_dependencies(slang-core slang-core-copy)
    endif()

    # gfx
    add_library(slang-gfx UNKNOWN IMPORTED GLOBAL)
    add_library(slang::gfx ALIAS slang-gfx)
    if (WIN32)
        set_target_properties(slang-gfx PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/gfx.lib")
        
        add_custom_target(slang-gfx-copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang-glslang.dll"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang-glslang.dll")
            add_dependencies(slang-gfx slang-gfx-copy)
    elseif (UNIX)
        set_target_properties(slang-gfx PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libgfx.so")
        
        add_custom_target(slang-gfx-copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libgfx.so"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libgfx.so")
            add_dependencies(slang-gfx slang-gfx-copy)
    endif()
endif()
