cmake_minimum_required(VERSION 3.20)
list(PREPEND CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_LIST_DIR}/../Modules"
  "${CMAKE_CURRENT_LIST_DIR}/../../../cmake/Modules")
include(CompilerRTDarwinUtils)

# Override compilation only for this architecture-selection contract.
macro(try_compile_only result)
  string(REPLACE "CAN_TARGET_osx_" "" arch "${result}")
  set_property(GLOBAL APPEND PROPERTY WHP_ARCH_PROBES "${arch}")
  set(${result} TRUE)
endmacro()

set(TEST_COMPILE_ONLY ON)
set(COMPILER_RT_DARWIN_NATIVE_ARCH_ONLY ON)
set(CMAKE_HOST_APPLE TRUE)
set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_CROSSCOMPILING FALSE)
set(CMAKE_OSX_ARCHITECTURES arm64e)
set(DARWIN_osx_ARCHS "i386;x86_64" CACHE STRING "old cache" FORCE)
darwin_test_archs(osx DARWIN_osx_ARCHS i386 x86_64 arm64 arm64e)
if(NOT "${DARWIN_osx_ARCHS}" STREQUAL "arm64e")
  message(FATAL_ERROR "ARM64e runtime inherited incompatible ABI: ${DARWIN_osx_ARCHS}")
endif()
get_property(probes GLOBAL PROPERTY WHP_ARCH_PROBES)
if(NOT "${probes}" STREQUAL "arm64e")
  message(FATAL_ERROR "ARM64e probed foreign targets: ${probes}")
endif()

# Cache must be invalidated when the selected ABI changes.
set(CMAKE_OSX_ARCHITECTURES arm64)
set_property(GLOBAL PROPERTY WHP_ARCH_PROBES "")
darwin_test_archs(osx DARWIN_osx_ARCHS i386 x86_64 arm64 arm64e)
if(NOT "${DARWIN_osx_ARCHS}" STREQUAL "arm64")
  message(FATAL_ERROR "ARM64 runtime inherited stale ABI: ${DARWIN_osx_ARCHS}")
endif()
get_property(probes GLOBAL PROPERTY WHP_ARCH_PROBES)
if(NOT "${probes}" STREQUAL "arm64")
  message(FATAL_ERROR "ARM64 probed foreign targets: ${probes}")
endif()

# An explicitly chosen two-slice build must still work.
set(CMAKE_OSX_ARCHITECTURES "arm64;arm64e")
darwin_test_archs(osx DARWIN_osx_ARCHS i386 x86_64 arm64 arm64e)
if(NOT "${DARWIN_osx_ARCHS}" STREQUAL "arm64;arm64e")
  message(FATAL_ERROR "Explicit universal slice missing: ${DARWIN_osx_ARCHS}")
endif()

# The host-only filter must never bleed into cross-target builds.
set(CMAKE_CROSSCOMPILING TRUE)
unset(DARWIN_osx_ARCHS CACHE)
set_property(GLOBAL PROPERTY WHP_ARCH_PROBES "")
darwin_test_archs(osx DARWIN_osx_ARCHS i386 x86_64 arm64 arm64e)
if(NOT "i386" IN_LIST DARWIN_osx_ARCHS)
  message(FATAL_ERROR "Native ABI filter leaked into cross compilation")
endif()

message(STATUS "Darwin native compiler-rt ABI isolation: passed")
