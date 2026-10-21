#!/usr/bin/env bash
# Build Clang + LLDB natively on macOS and test both Clang compilation and
# LLDB's embedded Clang expression compiler. No arm64e host ABI assumption.
set -euo pipefail

if [[ "$(uname -s)" != Darwin ]]; then
  echo "error: this smoke test must run on a macOS host" >&2
  exit 2
fi

source_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-${TMPDIR:-/tmp}/whp-lldb-macos-host}"
host_arch="$(uname -m)"
case "$host_arch" in
  arm64|x86_64) ;;
  *)
    echo "error: unsupported macOS host ABI: $host_arch" >&2
    exit 2 ;;
esac

for tool in cmake ninja xcrun grep tee; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "error: missing $tool" >&2
    exit 2
  fi
done

# Do not mix e.g. iOS SDKs, arm64e-only flags or foreign ELF targets
# into a normal native macOS LLDB host build.
sysroot="$(xcrun --sdk macosx --show-sdk-path)"
# The SDKROOT environment is also consumed by Clang/LLDB's expression driver.
# Override an inherited iPhoneOS/iPhoneSimulator SDK for this macOS *host* test.
if [[ ! -d "$sysroot" ]]; then
  echo "error: xcrun did not select a valid macOS SDK: $sysroot" >&2
  exit 2
fi
case "$(cd "$sysroot" && pwd -P)" in
  */MacOSX*.sdk) ;;
  *)
    echo "error: LLDB host smoke selected a non-macOS SDK: $sysroot" >&2
    exit 2 ;;
esac
export SDKROOT="$sysroot"
host_cc="$(xcrun --sdk macosx --find clang)"
host_cxx="$(xcrun --sdk macosx --find clang++)"
deployment="${WHP_MACOS_DEPLOYMENT_TARGET:-11.0}"
if [[ "$host_arch" == "arm64" ]]; then
  if ! /usr/bin/awk -v v="$deployment" 'BEGIN {exit !(v+0 >= 11.0)}'; then
    echo "error: macOS arm64 requires a deployment target >= 11.0" >&2
    exit 2
  fi
fi

echo "Host macOS ABI: $host_arch; deployment target: $deployment"
echo "Clang host compiler: $host_cxx"
echo "macOS SDK: $sysroot"
echo "Build directory: $build_dir"

# Check the Apple SDK's target macros before an expensive LLVM/LLDB build.
# TARGET_OS_MAC alone is not sufficient: it can include iOS targets.
if ! "$host_cc" -arch "$host_arch" -isysroot "$sysroot" \
        "-mmacosx-version-min=$deployment" -x c -fsyntax-only - <<'WHP_LLDB_HOST_SDK'
#include <TargetConditionals.h>
#if !defined(TARGET_OS_OSX) || !TARGET_OS_OSX || \
    (defined(TARGET_OS_IOS) && TARGET_OS_IOS) || \
    (defined(TARGET_OS_MACCATALYST) && TARGET_OS_MACCATALYST) || \
    (defined(TARGET_OS_SIMULATOR) && TARGET_OS_SIMULATOR)
#error LLDB host bootstrap must not use iOS/iPadOS SDK headers
#endif
WHP_LLDB_HOST_SDK
then
  echo "error: Apple SDK target macros disagree with the LLDB macOS host" >&2
  exit 2
fi

cmake -S "$source_root/llvm" -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER="$host_cc" \
  -DCMAKE_CXX_COMPILER="$host_cxx" \
  -DLLVM_MACOS_HOST_SDK_RESOLVE=ON \
  -DCMAKE_OSX_SYSROOT="$sysroot" \
  -DCMAKE_OSX_ARCHITECTURES="$host_arch" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  '-DLLVM_ENABLE_PROJECTS=clang;lldb' \
  '-DLLVM_TARGETS_TO_BUILD=AArch64;X86' \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLDB_INCLUDE_TESTS=OFF \
  -DLLDB_ENUM_WARNINGS_AS_ERRORS=ON \
  -DLLDB_ENABLE_PYTHON=OFF \
  -DLLDB_ENABLE_SWIG=OFF \
  -DLLDB_ENABLE_LUA=OFF \
  -DLLDB_ENABLE_LIBEDIT=OFF \
  -DLLDB_ENABLE_CURSES=OFF \
  -DLLDB_ENABLE_TREESITTER=OFF \
  -DLLDB_ENABLE_PROTOCOL_SERVERS=OFF \
  -DLLDB_USE_SYSTEM_DEBUGSERVER=ON

# A CI agent with 7 GB RAM should not run an unbounded Ninja job count.
cmake --build "$build_dir" --target clang lldb --parallel "${WHP_LLDB_JOBS:-2}"

compiler="$build_dir/bin/clang++"
debugger="$build_dir/bin/lldb"
if [[ ! -x "$compiler" || ! -x "$debugger" ]]; then
  echo "error: missing built clang++ or lldb" >&2
  exit 1
fi
"$compiler" --version
"$debugger" --version

# Compile LLDB private headers in both Apple SDK include orders, with C++23.
header_flags=(
  -std=c++23 -fsyntax-only -Werror
  -arch "$host_arch" -isysroot "$sysroot"
  "-mmacosx-version-min=$deployment"
  -I "$source_root"
  -I "$source_root/lldb/include"
  -I "$source_root/llvm/include"
  -I "$source_root/clang/include"
  -I "$build_dir/include"
  -I "$build_dir/tools/lldb/include"
  -I "$build_dir/tools/clang/include"
)
sdk_first="$build_dir/lldb-private-sdk-first.cpp"
private_first="$build_dir/lldb-private-lldb-first.cpp"
cat > "$sdk_first" <<'CPP'
#include <mach/mach.h>
#include <pthread.h>
#include <memory>
#include <optional>
#include <type_traits>
#include "lldb/lldb-private-types.h"
#include "lldb/lldb-private-interfaces.h"
#include "lldb/Core/EmulateInstruction.h"
#include "lldb/source/Plugins/ExpressionParser/Clang/ClangUserExpression.h"
#include "lldb/source/Plugins/TypeSystem/Clang/TypeSystemClang.h"
static_assert(std::is_destructible_v<
              lldb_private::SingleStepBreakpointLocationsPredictor>);
static_assert(std::is_destructible_v<
              lldb_private::ClangUserExpression::ClangUserExpressionHelper>);
static_assert(std::is_destructible_v<
              lldb_private::ScratchTypeSystemClang>);
CPP
cat > "$private_first" <<'CPP'
#include "lldb/lldb-private-types.h"
#include "lldb/lldb-private-interfaces.h"
#include "lldb/Core/EmulateInstruction.h"
#include "lldb/source/Plugins/ExpressionParser/Clang/ClangUserExpression.h"
#include "lldb/source/Plugins/TypeSystem/Clang/TypeSystemClang.h"
#include <mach/mach.h>
#include <pthread.h>
#include <memory>
#include <optional>
#include <type_traits>
static_assert(std::is_destructible_v<
              lldb_private::SingleStepBreakpointLocationsPredictor>);
static_assert(std::is_destructible_v<
              lldb_private::ClangUserExpression::ClangUserExpressionHelper>);
static_assert(std::is_destructible_v<
              lldb_private::ScratchTypeSystemClang>);
CPP
for private_header_smoke in "$sdk_first" "$private_first"; do
  echo "Checking LLDB private headers with Apple SDK: $private_header_smoke"
  "$host_cxx" "${header_flags[@]}" "$private_header_smoke"
done

# Check that the generated tools actually match the native macOS slice.
for artifact in "$compiler" "$debugger"; do
  if ! xcrun lipo -archs "$artifact" | grep -Eq "(^| )$host_arch( |$)"; then
    echo "error: native $host_arch Mach-O slice missing from $artifact" >&2
    exit 1
  fi
done

smoke_src="$build_dir/lldb-macos-expression-smoke.cpp"
smoke_bin="$build_dir/lldb-macos-expression-smoke"
cat > "$smoke_src" <<'CPP'
#include <cstdio>
__attribute__((noinline)) int lldb_smoke_anchor(int value) {
  volatile int observed = value;
  return observed + 1;
}
int main() {
  int result = lldb_smoke_anchor(41);
  std::printf("result=%d\n", result);
  return result == 42 ? 0 : 1;
}
CPP
"$compiler" -std=c++17 -g -O0 -arch "$host_arch" \
  -isysroot "$sysroot" "$smoke_src" -o "$smoke_bin"
"$smoke_bin"
xcrun lipo -archs "$smoke_bin" | grep -Eq "(^| )$host_arch( |$)"

# System debugserver avoids requiring a local lldb_codesign identity.
# Expression evaluation uses LLDB's embedded Clang compiler, not a
# shell-out to clang++. Capture the entire debugger output for diagnosis.
lldb_log="$build_dir/lldb-macos-expression-smoke.log"
"$debugger" --batch --file "$smoke_bin" \
  -o "breakpoint set --name lldb_smoke_anchor" \
  -o "run" \
  -o "expression -- 40 + 2" \
  -o "quit" 2>&1 | tee "$lldb_log"

if ! grep -Eq '\(int\)[[:space:]]+\$[0-9]+[[:space:]]+=[[:space:]]+42' "$lldb_log"; then
  echo "error: LLDB expression compilation did not produce 42" >&2
  exit 1
fi
echo "PASS: native macOS clang++ compilation and LLDB Clang expression"
