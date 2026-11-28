# Host executable allocator selection. Never change runtime/guest ABI.
set(LLVM_HOST_ALLOCATOR "system" CACHE STRING "Allocator for native LLVM executables")
set_property(CACHE LLVM_HOST_ALLOCATOR PROPERTY STRINGS system jemalloc)
set(LLVM_JEMALLOC_LIBRARY "" CACHE FILEPATH "Explicit native jemalloc dynamic library")

if(LLVM_HOST_ALLOCATOR STREQUAL "system")
  return()
endif()
if(NOT LLVM_HOST_ALLOCATOR STREQUAL "jemalloc")
  message(FATAL_ERROR "LLVM_HOST_ALLOCATOR must be system or jemalloc")
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "LLVM jemalloc host allocator currently requires a native Darwin build")
endif()
if(LLVM_USE_SANITIZER OR LLVM_ENABLE_RPMALLOC OR LLVM_INTEGRATED_CRT_ALLOC OR
   LLVM_ENABLE_LLVM_LIBC)
  message(FATAL_ERROR "LLVM jemalloc cannot be combined with sanitizers or another allocator replacement")
endif()
if(NOT IS_ABSOLUTE "${LLVM_JEMALLOC_LIBRARY}" OR
   NOT EXISTS "${LLVM_JEMALLOC_LIBRARY}" OR
   IS_DIRECTORY "${LLVM_JEMALLOC_LIBRARY}" OR
   NOT LLVM_JEMALLOC_LIBRARY MATCHES "\\.dylib$" OR
   LLVM_JEMALLOC_LIBRARY MATCHES "[,;\n\r]")
  message(FATAL_ERROR "LLVM_JEMALLOC_LIBRARY must identify an existing absolute jemalloc dylib")
endif()
file(REAL_PATH "${LLVM_JEMALLOC_LIBRARY}" _llvm_jemalloc_dylib)
if(_llvm_jemalloc_dylib MATCHES "[,;\n\r]")
  message(FATAL_ERROR "Resolved jemalloc path contains unsupported linker separators")
endif()
get_filename_component(_llvm_jemalloc_dir "${_llvm_jemalloc_dylib}" DIRECTORY)

# Retain the dylib even with dead-stripping enabled. The native malloc zone
# implementation must own ordinary C and C++ allocations; a loadable but
# inactive library is not an acceptable allocator provider.
set(_llvm_jemalloc_link_option "LINKER:-needed_library,${_llvm_jemalloc_dylib}")
include(CMakePushCheckState)
include(CheckCXXSourceRuns)
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_LINK_OPTIONS
  "${_llvm_jemalloc_link_option}" "LINKER:-rpath,${_llvm_jemalloc_dir}")
unset(LLVM_JEMALLOC_ZONE_WORKS CACHE)
check_cxx_source_runs([=[
#include <cstdlib>
#include <cstring>
#include <malloc/malloc.h>
#include <new>

static bool owned(const void *p) {
  if (!p)
    return false;
  auto *zone = malloc_zone_from_ptr(p);
  return zone && zone->zone_name &&
         std::strcmp(zone->zone_name, "jemalloc_zone") == 0;
}

int main() {
  void *(*volatile allocate)(size_t) = std::malloc;
  void *p = allocate(4096);
  bool c = owned(p);
  std::free(p);
  void *q = ::operator new(4096, std::nothrow);
  bool cpp = owned(q);
  ::operator delete(q);
  return c && cpp ? 0 : 1;
}
]=] LLVM_JEMALLOC_ZONE_WORKS)
cmake_pop_check_state()
if(NOT LLVM_JEMALLOC_ZONE_WORKS)
  message(FATAL_ERROR
    "Selected jemalloc failed the native Darwin allocation/ownership probe; "
    "use a compatible zone-enabled dylib or LLVM_HOST_ALLOCATOR=system")
endif()
message(STATUS "LLVM host allocator: jemalloc (${_llvm_jemalloc_dylib})")

function(llvm_link_host_allocator target)
  target_link_options(${target} PRIVATE "${_llvm_jemalloc_link_option}")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${_llvm_jemalloc_dylib}")
  set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH "${_llvm_jemalloc_dir}")
  set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${_llvm_jemalloc_dir}")
endfunction()
