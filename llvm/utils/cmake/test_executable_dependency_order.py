#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check that executable-specific LLVM CMake dependencies do not leak.

Run directly with Python 3 and CMake; no LLVM compilation is required.
The fixture includes the actual generate_llvm_objects macro from AddLLVM.cmake
and recreates the prior leak as a negative control.
"""

from pathlib import Path
import subprocess
import tempfile

LLVM = Path(__file__).resolve().parents[2]
source = (LLVM / "cmake/modules/AddLLVM.cmake").read_text(encoding="utf-8")
start = source.index("macro(generate_llvm_objects name)")
end = source.index("\nendmacro()", start) + len("\nendmacro()")
generator = source[start:end]
exe_start = source.index("macro(add_llvm_executable name)")
exe_end = source.index("\nendmacro(add_llvm_executable name)", exe_start)
executable = source[exe_start:exe_end]

assert "list(APPEND LLVM_COMMON_DEPENDS ${ARG_DEPENDS})" not in generator
assert "set(_llvm_executable_depends ${LLVM_COMMON_DEPENDS} ${ARG_DEPENDS})" in executable
assert "setup_dependency_debugging(${name} ${_llvm_executable_depends})" in executable
assert "add_dependencies(${name} ${_llvm_executable_depends})" in executable
assert "add_dependencies(${objlib} ${_llvm_executable_depends})" in executable

def run_case(macro: str, *, should_pass: bool) -> None:
    with tempfile.TemporaryDirectory(prefix="llvm-cmake-order-") as temp:
        root = Path(temp)
        cmake_lists = """
cmake_minimum_required(VERSION 3.20)
project(LLVMDependencyIsolation LANGUAGES NONE)
set(LLVM_ENABLE_OBJLIB OFF)
set(LLVM_TOOL_LLVM_DRIVER_BUILD OFF)
set(LLVM_COMMON_DEPENDS shared)
macro(llvm_process_sources output)
  set(${output} ${ARGN})
endmacro()
""" + macro + """
generate_llvm_objects(first first.cpp DEPENDS first_only)
if(NOT "${LLVM_COMMON_DEPENDS}" STREQUAL "shared")
  message(FATAL_ERROR "first executable leaked a target-only prerequisite")
endif()
generate_llvm_objects(second second.cpp DEPENDS second_only)
if(NOT "${LLVM_COMMON_DEPENDS}" STREQUAL "shared")
  message(FATAL_ERROR "second executable leaked a target-only prerequisite")
endif()
"""
        (root / "CMakeLists.txt").write_text(cmake_lists, encoding="utf-8")
        result = subprocess.run(
            ["cmake", "-S", str(root), "-B", str(root / "build")],
            capture_output=True, text=True,
        )
        if (result.returncode == 0) != should_pass:
            raise AssertionError(
                "Unexpected dependency-isolation result:\n"
                + result.stdout + "\n" + result.stderr
            )
        if not should_pass:
            assert "leaked a target-only prerequisite" in result.stderr

run_case(generator, should_pass=True)
needle = "  llvm_process_sources( ALL_FILES ${ARG_UNPARSED_ARGUMENTS} )"
assert generator.count(needle) == 1
old_generator = generator.replace(
    needle, needle + "\n  list(APPEND LLVM_COMMON_DEPENDS ${ARG_DEPENDS})"
)
run_case(old_generator, should_pass=False)
print("LLVM executable dependency isolation: verified")
