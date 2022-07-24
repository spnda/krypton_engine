# Simple macro that defines some generic compiler flags.
macro(compiler_flags)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})

  if(NOT PARAM_TARGET STREQUAL "")
    if(MSVC)
      # Microsoft not properly setting __cplusplus by default. AVX should be supported by any AMD/Intel CPU newer than 2011/2012. setting
      # /external makes warnings/diagnostic information not be printed for all headers included with angle-brackets.

      # cmake-format: off
      target_compile_options(${PARAM_TARGET} PRIVATE /Zc:__cplusplus /EHsc /W4 /MP /GF /utf-8 /arch:AVX /external:W0 /external:anglebrackets)
      # cmake-format: on

      # /Od for debug disables optimizations, speeding up compile times and allowing us to analyse the code better. /Ox for release is a
      # subset of /O2 and introduces a few more flags for speed optimizations.
      target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:DEBUG>:/analyze /sdl /Od /Zf>)
      target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:RELEASE>:/O2 /Ot /Ob2 /Oi>)
    else()
      # Clang, AppleClang, or GCC

      # cmake-format: off
      target_compile_options(${PARAM_TARGET} PRIVATE -Wall -Wextra -Wmicrosoft -Wold-style-cast -Wno-unused-parameter -pedantic -Og)
      # cmake-format: on

      if(KRYPTON_BUILD_TYPE_UPPER STREQUAL "DEBUG")
        target_compile_options(${PARAM_TARGET} PRIVATE -O0)
      else()
        target_compile_options(${PARAM_TARGET} PRIVATE -O3)
      endif()
      target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:DEBUG>:-O0>)
      target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:RELEASE>:-O3>)
    endif()
  endif()
endmacro()
