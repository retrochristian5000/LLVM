# Native Apple-host LLVM default target validation.
#
# A Darwin *kernel* triple is valid for LLVM_HOST_TRIPLE, but compiler
# products should default to an explicit macOS deployment target when the
# native macOS bootstrap opts into this guard.  Keep it independent of SDK
# discovery so it can be tested on non-Apple CI hosts.
function(llvm_check_native_macos_default_triple target_triple deployment_target)
  # CMake's regex language does not support {1,2} repetition quantifiers.
  # Write both mandatory and optional dot-version components explicitly.
  if(NOT "${target_triple}" MATCHES
     "^(aarch64|arm64e?|x86_64)-apple-macosx([0-9]+(\\.[0-9]+)(\\.[0-9]+)?)$")
    message(FATAL_ERROR
      "Native macOS LLVM default target must be versioned -apple-macosx, "
      "not Darwin, iOS, iPadOS, Catalyst or simulator: ${target_triple}")
  endif()
  set(target_version "${CMAKE_MATCH_2}")
  if(NOT "${target_version}" VERSION_EQUAL "${deployment_target}")
    message(FATAL_ERROR
      "Native macOS LLVM default triple version (${target_version}) "
      "does not match deployment target (${deployment_target})")
  endif()
  message(STATUS "LLVM native macOS default target: ${target_triple}")
endfunction()
