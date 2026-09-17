#!/usr/bin/env python3

from pathlib import Path
import argparse

HOST_CPP = Path("llvm/lib/TargetParser/Host.cpp")
HOST_H = Path("llvm/include/llvm/TargetParser/Host.h")
HOST_TEST = Path("llvm/unittests/TargetParser/Host.cpp")
APPLE_START = "#elif defined(__APPLE__) && (defined(__arm__) || defined(__aarch64__))\n"
APPLE_END = "#elif defined(_AIX)\n"


def apple_block(text: str) -> str:
    start = text.index(APPLE_START)
    end = text.index(APPLE_END, start)
    return text[start:end]


def verify_safe_source() -> None:
    block = apple_block(HOST_CPP.read_text())
    assert "Default to the newest CPU we know about" not in block, (
        "unknown Apple CPU families still select the newest known CPU"
    )
    assert (
        'sysctlbyname("hw.cpufamily", &Family, &Length, nullptr, 0) != 0 ||'
        in block
    ), "hw.cpufamily query failure is still unchecked"
    assert "Length != sizeof(Family)" in block
    assert "return detail::getHostCPUNameForAppleARM(Family);" in block


def expect_red() -> None:
    try:
        verify_safe_source()
    except AssertionError as exc:
        print(f"RED confirmed: {exc}")
        return
    raise SystemExit(
        "Apple host CPU safety check unexpectedly passed before the correction"
    )


def patch_header() -> None:
    text = HOST_H.read_text()
    if "#include <cstdint>\n" not in text:
        old = '#include "llvm/Support/Compiler.h"\n#include <string>\n'
        new = '#include "llvm/Support/Compiler.h"\n#include <cstdint>\n#include <string>\n'
        if text.count(old) != 1:
            raise SystemExit("Host.h include anchor changed")
        text = text.replace(old, new, 1)

    old = """LLVM_ABI StringRef getHostCPUNameForARM(uint64_t PrimaryCpuInfo,
                                        ArrayRef<uint64_t> UniqueCpuInfos);
"""
    new = old + """/// Map Darwin's hw.cpufamily value to an LLVM CPU name.
LLVM_ABI StringRef getHostCPUNameForAppleARM(uint32_t CPUFamily);
"""
    if "getHostCPUNameForAppleARM" not in text:
        if text.count(old) != 1:
            raise SystemExit("Host.h ARM helper declaration anchor changed")
        text = text.replace(old, new, 1)
    HOST_H.write_text(text)


def patch_source() -> None:
    text = HOST_CPP.read_text()
    start = text.index(APPLE_START)
    end = text.index(APPLE_END, start)
    replacement = """#elif defined(__APPLE__) && (defined(__arm__) || defined(__aarch64__))
StringRef sys::getHostCPUName() {
  uint32_t Family = 0;
  size_t Length = sizeof(Family);
  if (sysctlbyname("hw.cpufamily", &Family, &Length, nullptr, 0) != 0 ||
      Length != sizeof(Family))
    return "generic";
  return detail::getHostCPUNameForAppleARM(Family);
}
"""
    text = text[:start] + replacement + text[end:]

    if "StringRef sys::detail::getHostCPUNameForAppleARM" not in text:
        helper = r'''

StringRef sys::detail::getHostCPUNameForAppleARM(uint32_t Family) {
  // Values mirror Darwin's CPUFAMILY_ARM_* constants. Keep the mapping pure
  // so unknown/future families cannot silently inherit a newer CPU model.
  switch (Family) {
  case 0xe73283ae: // ARM_9
    return "arm920t"; // or arm926ej-s
  case 0x8ff620d8: // ARM_11
    return "arm1136jf-s";
  case 0x53b005f5: // ARM_XSCALE
    return "xscale";
  case 0xbd1b0ae9: // ARM_12, apparently unused by the kernel
    return "generic";
  case 0x0cc90e64: // ARM_13
    return "cortex-a8";
  case 0x96077ef1: // ARM_14
    return "cortex-a9";
  case 0xa8511bca: // ARM_15
    return "cortex-a7";
  case 0x1e2d6381: // SWIFT
    return "swift";
  case 0x37a09642: // CYCLONE
    return "apple-a7";
  case 0x2c91a47e: // TYPHOON
    return "apple-a8";
  case 0x92fb37c8: // TWISTER
    return "apple-a9";
  case 0x67ceee93: // HURRICANE
    return "apple-a10";
  case 0xe81e7ef6: // MONSOON_MISTRAL
    return "apple-a11";
  case 0x07d34b9f: // VORTEX_TEMPEST
    return "apple-a12";
  case 0x462504d2: // LIGHTNING_THUNDER
    return "apple-a13";
  case 0x1b588bb3: // FIRESTORM_ICESTORM: A14 / M1 share a family
    return "apple-m1";
  case 0xda33d83d: // BLIZZARD_AVALANCHE: A15 / M2 share a family
    return "apple-m2";
  case 0x8765edea: // EVEREST_SAWTOOTH: A16
    return "apple-a16";
  case 0xfa33415e: // IBIZA: M3
  case 0x72015832: // PALMA: M3 Max
  case 0x5f4dea93: // LOBOS: M3 Pro
    return "apple-m3";
  case 0x2876f5b5: // COLL: A17 Pro
    return "apple-a17";
  case 0x6f5129ac: // DONAN: M4
  case 0x17d5b93a: // BRAVA: M4 Pro / Max
    return "apple-m4";
  case 0x75d4acb9: // TAHITI: A18 Pro
  case 0x204526d0: // TUPAI: A18
    return "apple-a18";
  case 0x1d5a87e8: // HIDRA: M5
  case 0xf76c5b1a: // SOTRA: M5 Pro / Max
    return "apple-m5";
  case 0xab345f09: // THERA: A19 Pro
  case 0x01d7a72b: // TILOS: A19
    return "apple-a19";
  default:
    return "generic";
  }
}
'''
        text = text.rstrip() + helper + "\n"
    HOST_CPP.write_text(text)


def patch_test() -> None:
    text = HOST_TEST.read_text().rstrip()
    if "AppleARMFamilyMapping" in text:
        return
    test = r'''

TEST(getDarwinHostCPUName, AppleARMFamilyMapping) {
  // Mac17,5 / A18 Pro: preserve the A-series identity. apple-a18 is
  // code-generation-compatible with the existing apple-m4 alias today.
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0x75d4acb9), "apple-a18");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0x204526d0), "apple-a18");

  // Do not collapse distinct A- and M-series families when Darwin gives us
  // enough information to tell them apart.
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0x8765edea), "apple-a16");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0xfa33415e), "apple-m3");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0x6f5129ac), "apple-m4");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0x1d5a87e8), "apple-m5");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0xab345f09), "apple-a19");

  // Unknown and unavailable family values must be conservative.
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0), "generic");
  EXPECT_EQ(sys::detail::getHostCPUNameForAppleARM(0xdeadbeef), "generic");
}
'''
    HOST_TEST.write_text(text + test + "\n")


def apply() -> None:
    patch_header()
    patch_source()
    patch_test()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("red", "apply", "verify"))
    args = parser.parse_args()
    if args.mode == "red":
        expect_red()
    elif args.mode == "apply":
        apply()
    else:
        verify_safe_source()


if __name__ == "__main__":
    main()
