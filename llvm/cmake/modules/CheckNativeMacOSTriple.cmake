# Native Apple-host LLVM target validation.
#
# In the opted-in native macOS bootstrap, both LLVM_HOST_TRIPLE and
# LLVM_DEFAULT_TARGET_TRIPLE must identify macOS at the selected deployment
# version. Bootstrap compilers may still report Darwin kernel triples.
# Keep this independent of SDK discovery for non-Apple CMake tests.
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

# Checking only the default leaves LLVM_HOST_TRIPLE free to retain a stale
# Darwin or iOS value. Validate the independently configured host as well.
function(llvm_check_native_macos_toolchain_triples host_triple default_triple deployment_target)
  llvm_check_native_macos_default_triple("${host_triple}" "${deployment_target}")
  llvm_check_native_macos_default_triple("${default_triple}" "${deployment_target}")
  message(STATUS "LLVM native macOS host triple: ${host_triple}")
  message(STATUS "LLVM native macOS default triple: ${default_triple}")
endfunction()
