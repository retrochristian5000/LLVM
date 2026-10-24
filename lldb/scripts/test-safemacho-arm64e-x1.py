#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Regress the Apple SDK/LLVM ARM64E_X1 macro collision in SafeMachO.h.

The portable fast test compiles the real LLDB wrapper against a minimal
fixture with the SDK's typed macro and LLVM's checked enum value.  The
native macOS host smoke test compiles the *actual* SDK and LLVM headers.
"""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
safe = ROOT / "lldb/include/lldb/Host/SafeMachO.h"
llvm = ROOT / "llvm/include/llvm/BinaryFormat/MachO.h"
safe_source = safe.read_text(encoding="utf-8")
llvm_source = llvm.read_text(encoding="utf-8")

def subtype_value(name):
    pattern = r"\b" + re.escape(name) + r"\s*=\s*(\d+)\b"
    matches = re.findall(pattern, llvm_source)
    assert len(matches) == 1, (name, matches)
    return int(matches[0])

assert subtype_value("CPU_SUBTYPE_ARM64E") == 2
assert subtype_value("CPU_SUBTYPE_ARM64E_X1") == 12
assert safe_source.count("#undef CPU_SUBTYPE_ARM64E_X1") == 1
assert (safe_source.index("#undef CPU_SUBTYPE_ARM64E_X1")
        < safe_source.index('#include "llvm/BinaryFormat/MachO.h"'))

cxx = shutil.which("clang++") or shutil.which("g++") or shutil.which("c++")
if cxx is None:
    raise SystemExit("error: C++ compiler required for SafeMachO regression")

with tempfile.TemporaryDirectory(prefix="whp-safemacho-x1-") as tmp:
    d = Path(tmp)
    (d / "mach").mkdir()
    (d / "llvm/BinaryFormat").mkdir(parents=True)

    (d / "mach/machine.h").write_text(
        "#pragma once\n"
        "typedef int cpu_subtype_t;\n"
        "#define CPU_SUBTYPE_ARM64E_X1 ((cpu_subtype_t) 12)\n",
        encoding="utf-8",
    )
    (d / "llvm/BinaryFormat/MachO.h").write_text(
        "#pragma once\n"
        "namespace llvm { namespace MachO {\n"
        "enum CPUSubTypeARM64 : unsigned {\n"
        "  CPU_SUBTYPE_ARM64E = 2,\n"
        "  CPU_SUBTYPE_ARM64E_X1 = 12,\n"
        "};\n"
        "} }\n",
        encoding="utf-8",
    )
    direct = d / "direct.cpp"
    direct.write_text(
        '#include "mach/machine.h"\n'
        '#include "llvm/BinaryFormat/MachO.h"\n',
        encoding="utf-8",
    )
    wrapped = d / "wrapped.cpp"
    wrapped.write_text(
        '#include "mach/machine.h"\n'
        '#include "lldb/Host/SafeMachO.h"\n'
        "#ifdef CPU_SUBTYPE_ARM64E_X1\n"
        "#error Apple SDK macro remained defined after SafeMachO\n"
        "#endif\n"
        "static_assert(llvm::MachO::CPU_SUBTYPE_ARM64E_X1 == 12);\n"
        "static_assert(llvm::MachO::CPU_SUBTYPE_ARM64E == 2);\n",
        encoding="utf-8",
    )

    args = [cxx, "-std=c++17", "-Werror", "-fsyntax-only",
            "-I", str(d), "-I", str(ROOT / "lldb/include")]
    negative = subprocess.run(
        [*args, str(direct)], capture_output=True, text=True, check=False
    )
    assert negative.returncode != 0, (
        "fixture must demonstrate the SDK macro breaks LLVM's enum"
    )
    positive = subprocess.run(
        [*args, str(wrapped)], capture_output=True, text=True, check=False
    )
    assert positive.returncode == 0, (
        "SafeMachO must prevent the macro collision:\n"
        + positive.stdout + positive.stderr
    )

print("LLDB SafeMachO arm64e.x1 SDK macro collision: verified")
