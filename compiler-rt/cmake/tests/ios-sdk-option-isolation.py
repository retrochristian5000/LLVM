#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Exercise compiler-rt's real iOS option block without an Apple SDK.

The mocked SDK locator counts calls; CMake option() still runs normally.
This catches irrelevant iPhoneOS warnings in macOS-only compiler-rt builds
while retaining automatic SDK detection for cross-platform builds.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile

SOURCE = Path(__file__).resolve().parents[1] / "base-config-ix.cmake"
cmake = shutil.which("cmake")
if not cmake:
    raise SystemExit("error: cmake is required for iOS SDK option regression")

source = SOURCE.read_text(encoding="utf-8")
start = source.index("  # Auto-detect iOS only if")
last = source.index(
    '  option(COMPILER_RT_ENABLE_IOS "Enable building for iOS"', start
)
end = source.index("\n", last)
policy = source[start:end]

# name, explicit option, iPhoneOS SDK present, expected enabled, expected probe,
# expected warning. Explicit ON preserves the old error-reporting behavior.
scenarios = (
    ("native-off-no-sdk", "OFF", False, "OFF", False, False),
    ("native-off-with-sdk", "OFF", True, "OFF", False, False),
    ("cross-on-no-sdk", "ON", False, "ON", True, True),
    ("cross-on-with-sdk", "ON", True, "ON", True, False),
    ("auto-no-sdk", None, False, "OFF", True, True),
    ("auto-with-sdk", None, True, "ON", True, False),
)

with tempfile.TemporaryDirectory(prefix="whp-compiler-rt-ios-") as td:
    for name, requested, available, enabled, probe, warning in scenarios:
        # Never call xcrun: CMake executes exactly the production option block
        # with an instrumented SDK discovery function and ordinary option().
        setup = (
            "cmake_minimum_required(VERSION 3.18)\n"
            "function(find_darwin_sdk_dir out_var sdk_name)\n"
            '  if(NOT sdk_name STREQUAL "iphoneos")\n'
            '    message(FATAL_ERROR "unexpected SDK probe: ${sdk_name}")\n'
            "  endif()\n"
            "  set_property(GLOBAL PROPERTY WHP_SDK_PROBED TRUE)\n"
            f'  set(${{out_var}} "{"/mock/iPhoneOS.sdk" if available else ""}" PARENT_SCOPE)\n'
            "endfunction()\n"
        )
        if requested is not None:
            setup += (
                f'set(COMPILER_RT_ENABLE_IOS {requested} CACHE BOOL "test")\n'
            )
        setup += (
            policy + "\n"
            "get_property(was_probed GLOBAL PROPERTY WHP_SDK_PROBED)\n"
            'message(STATUS "WHP_IOS_RESULT=${COMPILER_RT_ENABLE_IOS};'
            'PROBE=${was_probed}")\n'
        )
        script = Path(td) / f"{name}.cmake"
        script.write_text(setup, encoding="utf-8")
        result = subprocess.run(
            [cmake, "-P", str(script)],
            text=True,
            capture_output=True,
            check=False,
        )
        output = result.stdout + result.stderr
        expected = f"WHP_IOS_RESULT={enabled};PROBE={'TRUE' if probe else ''}"
        assert result.returncode == 0, f"{name}: {output}"
        assert expected in output, f"{name}: expected {expected!r}: {output}"
        observed_warning = "iOS SDK not found!" in output
        assert observed_warning == warning, (
            f"{name}: expected iOS warning={warning}: {output}"
        )

print("compiler-rt native macOS/iPhoneOS SDK option isolation: passed")
