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
message(STATUS "Native macOS default-target platform/version contract: passed")
