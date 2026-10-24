#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Check production LLVM CMake syntax and compiler-rt Darwin SDK path handling.

CMake semicolon-separated lists break an unquoted SDK path inside if(EXISTS).
Run actual CMake syntax checks, not a regex-only approximation.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
CONFIG = ROOT / "compiler-rt/cmake/base-config-ix.cmake"
cmake = shutil.which("cmake")
if not cmake:
    raise SystemExit("error: cmake is required")

source = CONFIG.read_text(encoding="utf-8")
start = source.index("    if (NOT OSX_SYSROOT OR NOT EXISTS ")
end = source.index("\n    endif()", start) + len("\n    endif()")
policy = source[start:end]
assert 'NOT EXISTS "${OSX_SYSROOT}"' in policy, (
    "compiler-rt lost the quoted SDK path guard"
)


def run(script: Path):
    return subprocess.run([cmake, "-P", str(script)],
                          capture_output=True, text=True, check=False)


with tempfile.TemporaryDirectory(prefix="whp-cmake-sysroot-") as directory:
    root = Path(directory)
    for label, sdk, exists, expected_found in (
        ("simple", "MacOSX.sdk", True, True),
        ("spaces", "Mac OS X.sdk", True, True),
        ("semicolon", "MacOSX;Testing.sdk", True, True),
        ("missing", "MacOSX;Missing.sdk", False, False),
        ("empty", "", False, False),
    ):
        path = root / sdk if sdk else None
        if exists:
            path.mkdir()
        value = str(path) if path else ""
        script = root / f"{label}.cmake"
        script.write_text(
            "cmake_minimum_required(VERSION 3.20)\n"
            + f"set(OSX_SYSROOT [==[{value}]==])\n"
            + policy + "\n"
            + 'message(STATUS "WHP_FLAG=${OSX_SYSROOT_FLAG}")\n',
            encoding="utf-8"
        )
        result = run(script)
        output = result.stdout + result.stderr
        assert result.returncode == 0, (
            f"{label}: invalid CMake expression:\n{output}"
        )
        assert ("Found OSX_SYSROOT:" in output) == expected_found, (
            label, output
        )
        if expected_found:
            assert f"WHP_FLAG=-isysroot{value}" in output, (label, output)
        else:
            assert "does not exist" in output, (label, output)

    # Negative control proves the exact pre-fix error, rather than letting
    # the regression succeed because CMake silently skipped the condition.
    bad = root / "unquoted.cmake"
    bad.write_text(
        f'set(OSX_SYSROOT [==[{root / "MacOSX;Testing.sdk"}]==])\n'
        + policy.replace('"${OSX_SYSROOT}"', "${OSX_SYSROOT}")
        + "\n", encoding="utf-8"
    )
    result = run(bad)
    assert result.returncode != 0
    assert "Unknown arguments specified" in result.stdout + result.stderr

    # Parse related production files without executing CMake's project()
    # or Apple-specific dependency discovery. This tests lexical syntax,
    # not full configure-time behavior.
    for rel in (
        "llvm/CMakeLists.txt",
        "llvm/cmake/config-ix.cmake",
        "compiler-rt/CMakeLists.txt",
        "compiler-rt/cmake/base-config-ix.cmake",
    ):
        origin = ROOT / rel
        assert origin.is_file(), f"missing pinned CMake file: {rel}"
        script = root / (rel.replace("/", "_") + ".cmake")
        script.write_text(
            "cmake_minimum_required(VERSION 3.20)\n"
            "if(FALSE)\n" + origin.read_text(encoding="utf-8")
            + "\nendif()\n", encoding="utf-8"
        )
        result = run(script)
        assert result.returncode == 0, (
            f"malformed CMake syntax in {rel}:\n"
            + result.stdout + result.stderr
        )

print("LLVM/compiler-rt CMake syntax and Darwin SDK arguments: passed")
