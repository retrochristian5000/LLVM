cmake_minimum_required(VERSION 3.20)
include("${CMAKE_CURRENT_LIST_DIR}/../modules/CheckNativeMacOSTriple.cmake")

if(DEFINED CHECK_INVALID_TRIPLE)
  llvm_check_native_macos_default_triple(
    "${CHECK_INVALID_TRIPLE}" "${CHECK_INVALID_DEPLOYMENT}")
  return()
endif()

foreach(case IN ITEMS
    "aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-macosx15.6|15.6"
    "arm64-apple-macosx11.0|11.0"
    "arm64e-apple-macosx26.0|26.0"
    "x86_64-apple-macosx13.7.1|13.7.1"
    "aarch64-apple-macosx26.0.0|26.0")
  string(REPLACE "|" ";" pair "${case}")
  list(GET pair 0 triple)
  list(GET pair 1 deployment)
  llvm_check_native_macos_default_triple("${triple}" "${deployment}")
endforeach()

foreach(case IN ITEMS
    "aarch64-apple-darwin25.0|26.0"
    "aarch64-apple-ios26.0|26.0"
    "aarch64-apple-ios26.0-simulator|26.0"
    "aarch64-apple-macosx26.0-macabi|26.0"
    "aarch64-apple-macosx26|26.0"
    "aarch64-apple-macosx26.0.0.0|26.0"
    "aarch64-apple-macosx25.0|26.0"
    "aarch64-apple-ipados26.0|26.0"
    "x86_64-unknown-macosx13.0|13.0")
  string(REPLACE "|" ";" pair "${case}")
  list(GET pair 0 triple)
  list(GET pair 1 deployment)
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DCHECK_INVALID_TRIPLE=${triple}"
      "-DCHECK_INVALID_DEPLOYMENT=${deployment}"
      -P "${CMAKE_CURRENT_LIST_FILE}"
    RESULT_VARIABLE exit_code
    OUTPUT_QUIET ERROR_QUIET)
  if(exit_code EQUAL 0)
    message(FATAL_ERROR "Invalid macOS triple accepted: ${case}")
  endif()
endforeach()
# The native host must be validated independently of a valid macOS default.
# Reject iOS, Darwin-kernel, Catalyst and mismatched deployment versions in
# either position. Exercise the same entry point as llvm/CMakeLists.txt.
if(DEFINED CHECK_INVALID_HOST_DEFAULT_PAIR)
  llvm_check_native_macos_toolchain_triples(
    "${CHECK_HOST_TRIPLE}" "${CHECK_DEFAULT_TRIPLE}"
    "${CHECK_DEPLOYMENT_TARGET}")
  return()
endif()

foreach(case IN ITEMS
    "aarch64-apple-macosx26.0|aarch64-apple-macosx26.0|26.0"
    "arm64e-apple-macosx26.0|arm64e-apple-macosx26.0|26.0"
    "x86_64-apple-macosx13.7|x86_64-apple-macosx13.7.0|13.7")
  string(REPLACE "|" ";" parts "${case}")
  list(GET parts 0 host_triple)
  list(GET parts 1 default_triple)
  list(GET parts 2 deployment_target)
  llvm_check_native_macos_toolchain_triples(
    "${host_triple}" "${default_triple}" "${deployment_target}")
endforeach()

foreach(case IN ITEMS
    "aarch64-apple-darwin25.0|aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-ios26.0|aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-ipados26.0|aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-macosx26.0-macabi|aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-macosx25.0|aarch64-apple-macosx26.0|26.0"
    "aarch64-apple-macosx26.0|aarch64-apple-ios26.0|26.0"
    "aarch64-apple-macosx26.0|aarch64-apple-macosx25.0|26.0")
  string(REPLACE "|" ";" parts "${case}")
  list(GET parts 0 host_triple)
  list(GET parts 1 default_triple)
  list(GET parts 2 deployment_target)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DCHECK_INVALID_HOST_DEFAULT_PAIR=1"
      "-DCHECK_HOST_TRIPLE=${host_triple}"
      "-DCHECK_DEFAULT_TRIPLE=${default_triple}"
      "-DCHECK_DEPLOYMENT_TARGET=${deployment_target}"
      -P "${CMAKE_CURRENT_LIST_FILE}"
    RESULT_VARIABLE exit_code OUTPUT_QUIET ERROR_QUIET)
  if(exit_code EQUAL 0)
    message(FATAL_ERROR "Invalid native macOS host/default pair accepted: ${case}")
  endif()
endforeach()
message(STATUS "Native macOS host/default platform/version contract: passed")
