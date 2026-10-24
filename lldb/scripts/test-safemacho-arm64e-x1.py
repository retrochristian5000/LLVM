#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Test Xcode 27 ARM64 X1 macro collisions using real LLVM declarations.

Compile a dependency-light slice extracted from the pinned MachO.h, not a
hand-maintained second implementation of LLVM's CPU subtype classification.
The same tested SafeMachO wrapper is used by LLDB against the real macOS SDK.
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

sdk_names = (
    "CPU_SUBTYPE_ARM64_X1",
    "CPU_SUBTYPE_ARM64E_X1",
    "CPU_SUBTYPE_ARM64_MAX",
    "CPU_SUBTYPE_HAS_ARM64E",
    "CPU_SUBTYPE_HAS_ARM64X1",
)
for name in sdk_names:
    assert safe_source.count("#undef " + name + "\n") == 1, name
    assert safe_source.index("#undef " + name + "\n") < safe_source.index(
        '#include "llvm/BinaryFormat/MachO.h"'
    ), name

def subtype_value(name):
    pattern = r"\b" + re.escape(name) + r"\s*=\s*(\d+)\b"
    matches = re.findall(pattern, llvm_source)
    assert len(matches) == 1, (name, matches)
    return int(matches[0])

assert subtype_value("CPU_SUBTYPE_ARM64E") == 2
assert subtype_value("CPU_SUBTYPE_ARM64_X1") == 3
assert subtype_value("CPU_SUBTYPE_ARM64E_X1") == 12
assert subtype_value("CPU_SUBTYPE_ARM64_MAX") == 20

# Extract declarations straight from the real LLVM header: the tests must
# break when the production implementation changes. The full MachO.h has
# substantial LLVM dependencies not needed for this focused macro test.
start = llvm_source.index("enum CPUSubTypeARM64 : uint32_t {")
end = llvm_source.index(
    "inline unsigned CPU_SUBTYPE_ARM64E_PTRAUTH_VERSION", start
)
declarations = llvm_source[start:end]
assert "inline constexpr bool CPU_SUBTYPE_HAS_ARM64E(uint32_t ST)" in declarations
assert "inline constexpr bool CPU_SUBTYPE_HAS_ARM64X1(uint32_t ST)" in declarations

cxx = shutil.which("clang++") or shutil.which("g++") or shutil.which("c++")
assert cxx, "a C++17 compiler is required"

with tempfile.TemporaryDirectory(prefix="whp-macho-x1-macros-") as tmp:
    folder = Path(tmp)
    (folder / "mach").mkdir()
    (folder / "llvm/BinaryFormat").mkdir(parents=True)
    (folder / "mach/machine.h").write_text(
        "#pragma once\n"
        "typedef int cpu_subtype_t;\n"
        "#define CPU_SUBTYPE_ARM64_X1 ((cpu_subtype_t) 3)\n"
        "#define CPU_SUBTYPE_ARM64E_X1 ((cpu_subtype_t) 12)\n"
        "#define CPU_SUBTYPE_ARM64_MAX ((cpu_subtype_t) 20)\n"
        "#define CPU_SUBTYPE_HAS_ARM64E(st) (((st) & 0xffffff) == 2 || "
        "((st) & 0xffffff) == 12)\n"
        "#define CPU_SUBTYPE_HAS_ARM64X1(st) (((st) & 0xffffff) == 3 || "
        "((st) & 0xffffff) == 12)\n",
        encoding="utf-8",
    )
    (folder / "llvm/BinaryFormat/MachO.h").write_text(
        "#pragma once\n"
        "#include <cstdint>\n"
        "namespace llvm { namespace MachO {\n"
        "enum { CPU_SUBTYPE_MASK = 0xff000000U };\n"
        + declarations +
        "} }\n",
        encoding="utf-8",
    )
    direct = folder / "direct.cpp"
    direct.write_text(
        '#include "mach/machine.h"\n'
        '#include "llvm/BinaryFormat/MachO.h"\n',
        encoding="utf-8",
    )
    wrapped = folder / "wrapped.cpp"
    wrapped.write_text(
        '#include "mach/machine.h"\n'
        '#include "lldb/Host/SafeMachO.h"\n'
        + "".join(
            f"#ifdef {name}\n"
            f"#error SDK macro {name} leaked across SafeMachO boundary\n"
            "#endif\n"
            for name in sdk_names
        )
        + "using namespace llvm::MachO;\n"
        "static_assert(CPU_SUBTYPE_ARM64_X1 == 3);\n"
        "static_assert(CPU_SUBTYPE_ARM64E_X1 == 12);\n"
        "static_assert(CPU_SUBTYPE_ARM64_MAX == 20);\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64E(2));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64E(12));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64E(0x80000002U));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64E(0x8f00000cU));\n"
        "static_assert(!CPU_SUBTYPE_HAS_ARM64E(3));\n"
        "static_assert(!CPU_SUBTYPE_HAS_ARM64E(0x8f000003U));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64X1(3));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64X1(12));\n"
        "static_assert(CPU_SUBTYPE_HAS_ARM64X1(0x8f00000cU));\n"
        "static_assert(!CPU_SUBTYPE_HAS_ARM64X1(2));\n"
        "static_assert(!CPU_SUBTYPE_HAS_ARM64X1(20));\n",
        encoding="utf-8",
    )

    args = [cxx, "-std=c++17", "-Werror", "-fsyntax-only",
            "-I", str(folder), "-I", str(ROOT / "lldb/include")]
    negative = subprocess.run(
        [*args, str(direct)], capture_output=True, text=True, check=False
    )
    assert negative.returncode != 0, (
        "fixture must first reproduce a real Apple macro collision"
    )
    positive = subprocess.run(
        [*args, str(wrapped)], capture_output=True, text=True, check=False
    )
    assert positive.returncode == 0, (
        "SafeMachO must isolate Apple's typed and function-like macros:\n"
        + positive.stdout + positive.stderr
    )

print("LLDB/LLVM Xcode 27 ARM64 X1 subtype-family macros: verified")
