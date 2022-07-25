# slang's macOS builds are only for x86_64 and CMake is shit in detecting the target architecture.
if((WIN32
    AND EXISTS "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64"
    OR UNIX
    AND EXISTS "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-64"
   )
   AND NOT APPLE)
  message(STATUS "Found slang")

  # slang
  add_library(slang-core SHARED IMPORTED GLOBAL)
  add_library(slang::core ALIAS slang-core)
  target_include_directories(slang-core SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/external/slang")
  if(WIN32)
    set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.dll")
    set_target_properties(slang-core PROPERTIES IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.lib")

    add_custom_target(
      slang-core-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang.dll"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang.dll")
    add_dependencies(slang-core slang-core-copy)

    add_custom_target(
      slang-glslang-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/windows-x64/release/slang-glslang.dll"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/slang-glslang.dll")
    add_dependencies(slang-core slang-glslang-copy)
  elseif(APPLE) # APPLE before UNIX!
    set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION
                                                "${CMAKE_SOURCE_DIR}/external/slang/bin/macosx-x64/release/libslang.dylib")

    add_custom_target(
      slang-core-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/macosx-x64/release/libslang.dylib"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libslang.dylib")
    add_dependencies(slang-core slang-core-copy)
  elseif(UNIX)
    set_target_properties(slang-core PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so")

    add_custom_target(
      slang-core-copy ALL
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/slang/bin/linux-x64/release/libslang.so"
              "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libslang.so")
    add_dependencies(slang-core slang-core-copy)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/external/glslang-release/" AND EXISTS "${CMAKE_SOURCE_DIR}/external/glslang-debug/")
  message(STATUS "Found glslang")

  # glslang
  add_library(glslang STATIC IMPORTED GLOBAL)
  add_library(glslang::glslang ALIAS glslang)

  if(KRYPTON_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    set(GLSLANG_LIB_DIR "${CMAKE_SOURCE_DIR}/external/glslang-debug/lib")
    if(WIN32)
      set(GLSLANG_LIB_POSTFIX "d.lib")
    endif()
  else()
    set(GLSLANG_LIB_DIR "${CMAKE_SOURCE_DIR}/external/glslang-release/lib")
    if(WIN32)
      set(GLSLANG_LIB_POSTFIX ".lib")
    endif()
  endif()

  if(WIN32)
    set(GLSLANG_LIB_PREFIX "")
  elseif(UNIX)
    set(GLSLANG_LIB_PREFIX "lib")
    set(GLSLANG_LIB_POSTFIX ".a")
  endif()

  # We just use the headers from glslang-release.
  target_include_directories(glslang SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/external/glslang-release/include")
  set_target_properties(glslang PROPERTIES IMPORTED_LOCATION "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}glslang${GLSLANG_LIB_POSTFIX}")

  # Libraries that depend on another library need to have their dependant listed after itself. This is how ld works.
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}MachineIndependent${GLSLANG_LIB_POSTFIX}")

  # Dependencies for MachineIndependent.lib
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}OSDependent${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}OGLCompiler${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}GenericCodeGen${GLSLANG_LIB_POSTFIX}")

  # HLSL and SPIR-V
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}HLSL${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}SPIRV${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}SPIRV-Tools-opt${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}SPIRV-Tools${GLSLANG_LIB_POSTFIX}")
  target_link_libraries(glslang INTERFACE "${GLSLANG_LIB_DIR}/${GLSLANG_LIB_PREFIX}SPVRemapper${GLSLANG_LIB_POSTFIX}")
endif()

if(APPLE AND EXISTS "${CMAKE_SOURCE_DIR}/external/metal-cpp/metal-cpp-beta")
  message(STATUS "Found metal-cpp")

  find_library(METAL_LIBRARY Metal)
  find_library(QUARTZCORE_LIBRARY QuartzCore)

  # metal-cpp is a header only external dependency
  add_library(metal-cpp INTERFACE)
  target_include_directories(metal-cpp SYSTEM INTERFACE "${CMAKE_SOURCE_DIR}/external/metal-cpp/metal-cpp-beta")
  target_link_libraries(metal-cpp INTERFACE ${METAL_LIBRARY})
  target_link_libraries(metal-cpp INTERFACE ${QUARTZCORE_LIBRARY})
endif()
