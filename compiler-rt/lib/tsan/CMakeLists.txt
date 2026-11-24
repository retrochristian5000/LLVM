# Build for the ThreadSanitizer runtime support library.

set(TSAN_CFLAGS ${SANITIZER_COMMON_CFLAGS})
# SANITIZER_COMMON_CFLAGS contains -fPIC, but it's performance-critical for
# TSan runtime to be built with -fPIE to reduce the number of register spills.
# On FreeBSD however it provokes linkage issue thus we disable it.
if(NOT CMAKE_SYSTEM MATCHES "FreeBSD")
  append_list_if(COMPILER_RT_HAS_FPIE_FLAG -fPIE TSAN_CFLAGS)
endif()
append_rtti_flag(OFF TSAN_CFLAGS)

if(COMPILER_RT_TSAN_DEBUG_OUTPUT)
  # Add extra debug information to TSan runtime. This configuration is rarely
  # used, but we need to support it so that debug output will not bitrot.
  list(APPEND TSAN_CFLAGS -DTSAN_DEBUG_OUTPUT=2)
endif()

# Objective-C++ is a separate frontend mode even when the C++ compiler
# is also Clang. Enable it only for the explicitly selected native macOS
# runtime; builtins, Go and other Darwin targets keep their existing graph.
option(COMPILER_RT_TSAN_OBJCXX "Enable native macOS Objective-C++ TSan hooks" OFF)
if(COMPILER_RT_TSAN_OBJCXX)
  if(NOT APPLE OR NOT COMPILER_RT_DARWIN_NATIVE_ARCH_ONLY OR
     NOT "${TSAN_SUPPORTED_OS}" STREQUAL "osx")
    message(FATAL_ERROR "TSan Objective-C++ requires the native macOS-only runtime profile")
  endif()
  if(NOT CMAKE_OBJCXX_COMPILER)
    set(CMAKE_OBJCXX_COMPILER "${CMAKE_CXX_COMPILER}" CACHE FILEPATH
        "Objective-C++ compiler for TSan")
  endif()
  if(CMAKE_CXX_COMPILER_TARGET AND NOT CMAKE_OBJCXX_COMPILER_TARGET)
    set(CMAKE_OBJCXX_COMPILER_TARGET "${CMAKE_CXX_COMPILER_TARGET}")
  endif()
  enable_language(OBJCXX)
  if(NOT CMAKE_OBJCXX_COMPILER_ID MATCHES "^(AppleClang|Clang)$")
    message(FATAL_ERROR "TSan Objective-C++ requires Clang's native Objective-C++ frontend")
  endif()
  set(CMAKE_OBJCXX_STANDARD 17)
  set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
  include(CheckOBJCXXSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state()
  list(APPEND CMAKE_REQUIRED_LIBRARIES objc)
  unset(TSAN_OBJCXX_COMPILES CACHE)
  check_objcxx_source_compiles([=[
#include <objc/objc.h>
#include <stddef.h>
extern "C" void objc_copyStruct(void *, const void *, ptrdiff_t, BOOL, BOOL);
int main() {
  int source = 1, destination = 0;
  const char *encoding = @encode(BOOL);
  objc_copyStruct(&destination, &source, sizeof(source), NO, NO);
  return !encoding[0] || destination != source;
}
]=] TSAN_OBJCXX_COMPILES)
  cmake_pop_check_state()
  if(NOT TSAN_OBJCXX_COMPILES)
    message(FATAL_ERROR "TSan Objective-C++ requires compatible SDK headers and libobjc")
  endif()
endif()

# Add the actual runtime library.
add_subdirectory(rtl)

# Build libcxx instrumented with TSan.
if(COMPILER_RT_LIBCXX_PATH AND
   COMPILER_RT_LIBCXXABI_PATH AND
   COMPILER_RT_TEST_COMPILER_ID STREQUAL "Clang" AND
   NOT ANDROID)
  set(libcxx_tsan_deps)
  foreach(arch ${TSAN_SUPPORTED_ARCH})
    get_target_flags_for_arch(${arch} TARGET_CFLAGS)
    set(LIBCXX_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libcxx_tsan_${arch})
    add_custom_libcxx(libcxx_tsan_${arch} ${LIBCXX_PREFIX}
      DEPS ${TSAN_RUNTIME_LIBRARIES}
      CFLAGS ${TARGET_CFLAGS} -fsanitize=thread
      CMAKE_ARGS -DRUNTIMES_EXECUTE_ONLY_CODE=${RUNTIMES_EXECUTE_ONLY_CODE}
      USE_TOOLCHAIN)
    list(APPEND libcxx_tsan_deps libcxx_tsan_${arch}-install-cmake326-workaround)
  endforeach()

  add_custom_target(libcxx_tsan DEPENDS ${libcxx_tsan_deps})
  set_target_properties(libcxx_tsan PROPERTIES FOLDER "Compiler-RT/Metatargets")
endif()

if(COMPILER_RT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif()
