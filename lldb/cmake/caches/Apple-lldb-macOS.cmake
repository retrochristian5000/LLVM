include(${CMAKE_CURRENT_LIST_DIR}/Apple-lldb-base.cmake)

set(LLDB_BUILD_FRAMEWORK ON CACHE BOOL "")
set(LLDB_NO_INSTALL_DEFAULT_RPATH ON CACHE BOOL "")
set(LLDB_SKIP_STRIP ON CACHE BOOL "")
# ARM64 macOS binaries require macOS 11.0 or newer. The historical 10.11
# preset can be kept for Intel-only builds, but not for native Apple Silicon
# or universal builds containing arm64. Check an explicit architecture
# selection first: an Apple Silicon host may intentionally cross-build x86_64.
set(_LLDB_MACOS_HAS_ARM64 OFF)
if(CMAKE_OSX_ARCHITECTURES)
  if(CMAKE_OSX_ARCHITECTURES MATCHES "(^|;)arm64(e)?(;|$)")
    set(_LLDB_MACOS_HAS_ARM64 ON)
  endif()
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|arm64e|aarch64)$")
  set(_LLDB_MACOS_HAS_ARM64 ON)
endif()

if(_LLDB_MACOS_HAS_ARM64)
  set(_LLDB_MACOS_DEFAULT_DEPLOYMENT_TARGET 11.0)
else()
  set(_LLDB_MACOS_DEFAULT_DEPLOYMENT_TARGET 10.11)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET
    "${_LLDB_MACOS_DEFAULT_DEPLOYMENT_TARGET}" CACHE STRING "")

# A command-line or inherited cache override must not quietly reintroduce
# a too-old deployment target. arm64e is deliberately not selected by default.
if(_LLDB_MACOS_HAS_ARM64 AND
   CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 11.0)
  message(FATAL_ERROR
    "Apple Silicon LLDB requires macOS deployment target >= 11.0; "
    "found ${CMAKE_OSX_DEPLOYMENT_TARGET}. Set "
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 or newer.")
endif()
unset(_LLDB_MACOS_HAS_ARM64)
unset(_LLDB_MACOS_DEFAULT_DEPLOYMENT_TARGET)

# Default install location on the enduser machine. On the build machine, use the
# DESTDIR environment variable in order to relocate the whole installation, e.g.:
# `DESTDIR=/tmp ninja install-distribution`
set(CMAKE_INSTALL_PREFIX /Applications/Xcode.app/Contents/Developer/usr CACHE STRING "")

# Install location for LLDB.framework on the enduser machine.
# DESTDIR will be an extra prefix.
set(LLDB_FRAMEWORK_INSTALL_DIR /Applications/Xcode.app/Contents/SharedFrameworks CACHE STRING "")

# Install location for externalized debug-info on the build machine.
# DESTDIR will be an extra prefix.
set(LLDB_DEBUGINFO_INSTALL_PREFIX /debuginfo CACHE STRING "")

set(LLVM_DISTRIBUTION_COMPONENTS
  lldb
  liblldb
  lldb-argdumper
  lldb-dap
  lldb-mcp
  darwin-debug
  debugserver
  CACHE STRING "")
