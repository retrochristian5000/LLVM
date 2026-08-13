//===-- llvm-nm-gnu-compat.cpp - GNU command-line compatibility -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Keep GNU command-line compatibility policy separate from llvm-nm's symbol
// implementation. Native llvm-nm behavior remains the default; when
// --gnu-compatible is present, overlapping GNU option semantics are translated
// before the normal llvm-nm entry point parses the command line.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/LLVMDriver.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/raw_ostream.h"
#include <cctype>
#include <string>
#include <vector>

using namespace llvm;

int llvm_nm_main_impl(int argc, char **argv,
                      const llvm::ToolContext &ToolContext);

namespace {

static bool getGNUFormat(StringRef Value, std::string &Canonical) {
  if (Value.empty())
    return false;

  const char First = static_cast<char>(
      std::tolower(static_cast<unsigned char>(Value.front())));
  switch (First) {
  case 'b':
    Canonical = "bsd";
    return true;
  case 's':
    Canonical = "sysv";
    return true;
  case 'p':
    Canonical = "posix";
    return true;
  case 'j':
    Canonical = "just-symbols";
    return true;
  default:
    return false;
  }
}

static int reportBadGNUFormat(StringRef ToolName, StringRef Value) {
  errs() << ToolName
         << ": error: GNU compatibility mode: output format '" << Value
         << "' must begin with b, s, p, or j\n";
  return 1;
}

static void appendGNUFormat(std::vector<std::string> &Args,
                            StringRef Canonical) {
  Args.emplace_back("--format=");
  Args.back().append(Canonical.data(), Canonical.size());
}

} // namespace

int llvm_nm_main(int argc, char **argv, const llvm::ToolContext &ToolContext) {
  // Match llvm-nm's normal OptTable parser by expanding response files before
  // deciding whether GNU compatibility was requested. This keeps
  // --gnu-compatible reliable when build systems place options in @FILE.
  BumpPtrAllocator Alloc;
  StringSaver Saver(Alloc);
  SmallVector<const char *, 0> ExpandedArgv;
  if (!cl::expandResponseFiles(argc, argv, nullptr, Saver, ExpandedArgv))
    return 1;

  bool GNUCompatible = false;
  bool ParseOptions = true;
  for (const char *RawArg : ExpandedArgv) {
    StringRef Arg(RawArg);
    if (ParseOptions && Arg == "--") {
      ParseOptions = false;
      continue;
    }
    if (ParseOptions && Arg == "--gnu-compatible")
      GNUCompatible = true;
  }

  if (!GNUCompatible)
    return llvm_nm_main_impl(argc, argv, ToolContext);

  std::vector<std::string> Storage;
  Storage.reserve(ExpandedArgv.size() + 1);
  Storage.emplace_back(argv[0]);

  ParseOptions = true;
  for (size_t I = 0; I < ExpandedArgv.size(); ++I) {
    StringRef Arg(ExpandedArgv[I]);

    if (!ParseOptions) {
      Storage.emplace_back(Arg.str());
      continue;
    }
    if (Arg == "--") {
      ParseOptions = false;
      Storage.emplace_back("--");
      continue;
    }
    if (Arg == "--gnu-compatible")
      continue;

    // GNU nm uses -s as the short spelling of --print-armap. Native llvm-nm
    // keeps -s <segment> <section> for its Darwin compatibility interface.
    if (Arg == "-s") {
      Storage.emplace_back("--print-armap");
      continue;
    }

    // GNU nm treats only the first character of the requested output format as
    // significant, case-insensitively. Canonicalize the value before handing
    // it to llvm-nm's stricter parser.
    if (Arg == "-f" || Arg == "--format") {
      if (I + 1 >= ExpandedArgv.size()) {
        Storage.emplace_back(Arg.str());
        continue;
      }
      StringRef Value(ExpandedArgv[++I]);
      std::string Canonical;
      if (!getGNUFormat(Value, Canonical))
        return reportBadGNUFormat(argv[0], Value);
      appendGNUFormat(Storage, Canonical);
      continue;
    }
    if (Arg.starts_with("-f") && !Arg.starts_with("--") && Arg.size() > 2) {
      StringRef Value = Arg.drop_front(2);
      std::string Canonical;
      if (!getGNUFormat(Value, Canonical))
        return reportBadGNUFormat(argv[0], Value);
      appendGNUFormat(Storage, Canonical);
      continue;
    }
    if (Arg.starts_with("--format=")) {
      StringRef Value = Arg.drop_front(StringRef("--format=").size());
      std::string Canonical;
      if (!getGNUFormat(Value, Canonical))
        return reportBadGNUFormat(argv[0], Value);
      appendGNUFormat(Storage, Canonical);
      continue;
    }

    Storage.emplace_back(Arg.str());
  }

  std::vector<char *> RewrittenArgv;
  RewrittenArgv.reserve(Storage.size());
  for (std::string &Arg : Storage)
    RewrittenArgv.push_back(Arg.data());

  return llvm_nm_main_impl(static_cast<int>(RewrittenArgv.size()),
                           RewrittenArgv.data(), ToolContext);
}
