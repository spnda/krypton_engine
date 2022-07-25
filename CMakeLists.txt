cmake_minimum_required(VERSION 3.24)

option(KRYPTON_USE_TRACY "Enable the Tracy profiler" OFF)

# Using 'INTERNAL' allows us to use ints for cache variables... There seems to be no other way
set(KRYPTON_TRACY_CALLSTACK_SIZE
    15
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

project("krypton_engine" LANGUAGES CXX)

if(APPLE)
  enable_language(OBJCXX)
endif()

# This speeds up build times drastically.
if(MSVC)
  add_compile_options(/MP)
  add_link_options(/incremental)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<0:>") # The $<0:> is a generator expression to remove the Debug/Release
                                                                      # folders.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")

if(KRYPTON_BUILD_TYPE_UPPER STREQUAL "DEBUG")
  add_compile_definitions(KRYPTON_DEBUG)
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
include(compiler_flags)

# We add this first as this provides targets other subdirectories require
add_subdirectory("${CMAKE_SOURCE_DIR}/external")
add_subdirectory("${CMAKE_SOURCE_DIR}/submodules")

add_subdirectory("${CMAKE_SOURCE_DIR}/src/assets")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/core")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/rapi")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/shaders")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/util")

option(KRYPTON_TESTS "Test the utility library of krypton with Catch2" OFF)
if(KRYPTON_TESTS)
  add_subdirectory("${CMAKE_SOURCE_DIR}/tests")
endif()
