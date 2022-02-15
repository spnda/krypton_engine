# slang doesn't provide builds for MacOS.
if(EXISTS "${CMAKE_SOURCE_DIR}/external/slang/bin" AND NOT APPLE)
  message(STATUS "Found slang")

  # slang
  add_library(slang-core UNKNOWN IMPORTED GLOBAL)
  add_library(slang::core ALIAS slang-core)
  set_property(TARGET slang-core PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/slang")
  if(WIN32)
    set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.lib")

    add_custom_target(
      slang-core-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.dll"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang.dll")
    add_dependencies(slang-core slang-core-copy)
  elseif(UNIX)
    set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so")

    add_custom_target(
      slang-core-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libslang.so")
    add_dependencies(slang-core slang-core-copy)
  endif()

  # gfx
  add_library(slang-gfx UNKNOWN IMPORTED GLOBAL)
  add_library(slang::gfx ALIAS slang-gfx)
  if(WIN32)
    set_target_properties(slang-gfx PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/gfx.lib")

    add_custom_target(
      slang-gfx-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang-glslang.dll"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang-glslang.dll")
    add_dependencies(slang-gfx slang-gfx-copy)
  elseif(UNIX)
    set_target_properties(slang-gfx PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libgfx.so")

    add_custom_target(
      slang-gfx-copy ALL COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libgfx.so"
                                 "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libgfx.so")
    add_dependencies(slang-gfx slang-gfx-copy)
  endif()
endif()
