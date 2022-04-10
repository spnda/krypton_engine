cmake_minimum_required(VERSION 3.21)

option(KRYPTON_USE_TRACY "Enable the Tracy profiler" OFF)

# Using 'INTERNAL' allows us to use ints for cache variables... There seems to be no other way
set(KRYPTON_TRACY_CALLSTACK_SIZE
    10
    CACHE INTERNAL "Set the size of the callstack for tracy to capture")

# This forces us to set variables explicitly in CACHE so that the option() command does not override the value.
cmake_policy(SET CMP0077 NEW)

# Automatically find our vcpkg.cmake toolchain file. We only do this on Windows, as on Linux or Mac the user could also use the package
# manager.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(vcpkg_file "$ENV{VCPKG_ROOT}\\scripts\\buildsystems\\vcpkg.cmake")
  cmake_path(SET vcpkg_file NORMALIZE "${vcpkg_file}")
  set(CMAKE_TOOLCHAIN_FILE
      "${vcpkg_file}"
      CACHE FILEPATH "")
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(
    WARNING
      "The CMAKE_TOOLCHAIN_FILE define and the VCPKG_ROOT environment variable are not defined, this could cause issues while searching for dependencies."
  )
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Debug"
      CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Normalize the CMAKE_BUILD_TYPE variable to be in PascalCase.
string(TOUPPER ${CMAKE_BUILD_TYPE} KRYPTON_BUILD_TYPE_UPPER)
if(KRYPTON_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  set(CMAKE_BUILD_TYPE "Debug")
elseif(KRYPTON_BUILD_TYPE_UPPER STREQUAL "RELEASE")
  set(CMAKE_BUILD_TYPE "Release")
elseif(KRYPTON_BUILD_TYPE_UPPER STREQUAL "RELWITHDEBINFO")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
elseif(KRYPTON_BUILD_TYPE_UPPER STREQUAL "MINSIZEREL")
  set(CMAKE_BUILD_TYPE "MinSizeRel")
else()
  message(FATAL_ERROR "The CMAKE_BUILD_TYPE variable is not set to a valid value, options are: Debug Release RelWithDebInfo MinSizeRel.")
endif()

project("krypton_engine")

enable_language(CXX)
if(APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<0:>") # The $<0:> is a generator expression to remove the Debug/Release
                                                                      # folders.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
if(MSVC) # cl.exe or clang-cl.exe
  # Microsoft not properly setting __cplusplus by default. AVX should be supported by any AMD/Intel CPU newer than 2011/2012. setting
  # /external makes warnings/diagnostic information not be printed for all headers included with angle-brackets.
  # cmake-format: off
  add_compile_options(/Zc:__cplusplus /EHsc /W4 /Ob2 /MP /arch:AVX /external:anglebrackets)
  # cmake-format: on

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # CMake auto sets /RTC1 which is incompatible with /O1 and /O2.
    add_compile_options(/analyze /sdl)
  else()
    add_compile_options(/O2)
  endif()
else()
  # Clang, AppleClang, or GCC
  add_compile_options(-Wall -Wextra -pedantic -Og)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Og)
  else()
    add_compile_options(-O3)
  endif()
endif()

set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/external")

# We set the TRACY_ENABLE variable accordingly.
if(KRYPTON_USE_TRACY)
  message(STATUS "Using tracy")
  add_compile_definitions(TRACY_ENABLE)
  add_compile_definitions(KRYPTON_USE_TRACY)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(add_files)
include(add_source_directory)

# We add this first as this provides targets other subdirectories require
add_subdirectory("${CMAKE_SOURCE_DIR}/src/external")
add_subdirectory("${CMAKE_SOURCE_DIR}/submodules")

add_subdirectory("${CMAKE_SOURCE_DIR}/src/assets")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/rapi")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/shaders")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/threading")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/util")

add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
