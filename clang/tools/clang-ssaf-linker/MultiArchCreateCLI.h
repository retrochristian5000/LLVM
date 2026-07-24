//===- MultiArchCreateCLI.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  Declares the CLI action class for `clang-ssaf-linker multi-arch create`:
//  bundles StaticLibrary/MultiArchStaticLibrary inputs into one
//  MultiArchStaticLibrary, or LUSummaryEncoding/MultiArchSharedLibrary
//  inputs into one MultiArchSharedLibrary. Every input is read through the
//  self-describing ArtifactEncoding, so the family (static vs. shared) is
//  inferred from the inputs rather than named on the command line; any
//  MultiArch* input is flattened into its per-architecture members, mirroring
//  `lipo -create`'s handling of thin-or-fat inputs.
//
//  The class is intentionally independent of the tool's cl::opt globals.
//  Every input it needs is passed via a Config struct at run() time, so
//  the class can be reused or unit-tested outside the driver.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_CLANG_SSAF_LINKER_MULTIARCHCREATECLI_H
#define LLVM_CLANG_TOOLS_CLANG_SSAF_LINKER_MULTIARCHCREATECLI_H

#include "clang/ScalableStaticAnalysis/Core/EntityLinker/MultiArchSharedLibrary.h"
#include "clang/ScalableStaticAnalysis/Core/EntityLinker/MultiArchStaticLibrary.h"
#include "clang/ScalableStaticAnalysis/Core/Model/BuildNamespace.h"
#include "clang/ScalableStaticAnalysis/Tool/Utils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace clang::ssaf {

/// Runs the `multi-arch create` action for `clang-ssaf-linker`.
class MultiArchCreateCLI {
public:
  /// Everything the action needs at runtime, threaded in from the CLI
  /// driver. StringRef/ArrayRef fields alias the driver's cl::opt storage
  /// and must remain valid for the duration of run().
  struct Config {
    llvm::ArrayRef<std::string> InputPaths;
    llvm::StringRef OutputPath;
    llvm::StringRef Namespace;
    bool Verbose = false;
    bool Time = false;
  };

  /// Orchestrator: validate → bundle → write. Non-recoverable errors call
  /// fail() from Tool/Utils.h and terminate the process.
  void run(llvm::TimerGroup &TG, const Config &Cfg);

private:
  /// Which family of artifact this bundle is being assembled from. Decided
  /// by the first input's ArtifactEncoding alternative; every later input
  /// must agree.
  enum class Family { Unknown, Static, Shared };

  /// Validates the output path and input paths.
  void validate(llvm::TimerGroup &TG);

  /// Reads each validated input file via readArtifactEncoding, classifies
  /// it into the static or shared family (flattening MultiArch* inputs into
  /// their per-architecture members), resolves the target namespace
  /// identity, and assembles the result.
  ///
  /// Terminates the process via fail() on any read error, mixed-family
  /// input, invalid input kind, namespace mismatch, or duplicate
  /// architecture slice.
  void bundle(llvm::TimerGroup &TG);

  /// Serializes whichever of StaticResult/SharedResult bundle() populated,
  /// via the generic writeArtifactEncoding entry point.
  void write(llvm::TimerGroup &TG);

  /// Moves each candidate's Member into Result.Members, failing on the
  /// first duplicate architecture slice (reporting both contributing source
  /// files). A member function (rather than a free function) so it retains
  /// this class's friend access to Result's and Candidate.Member's private
  /// fields. Defined, and only ever instantiated, in the .cpp.
  template <typename ResultT, typename CandidateT>
  void insertCandidates(ResultT &Result, std::vector<CandidateT> &Candidates);

  /// Prints one indented note to stderr when Cfg.Verbose is set.
  template <typename... Ts>
  void info(unsigned Level, const char *Fmt, Ts &&...Args) const {
    if (Cfg.Verbose) {
      llvm::WithColor::note()
          << std::string(Level * IndentationWidth, ' ') << "- "
          << llvm::formatv(Fmt, std::forward<Ts>(Args)...) << "\n";
    }
  }

  static constexpr unsigned IndentationWidth = 2;

  // Configuration set by run() before dispatching to phase methods.
  Config Cfg;

  // State populated during validate() and consumed by later phases.
  FormatFile OutputFile;
  std::vector<FormatFile> InputFiles;

  // State populated during bundle() and consumed by write(). Exactly one of
  // these is populated, decided by Family.
  Family ResultFamily = Family::Unknown;
  std::optional<MultiArchStaticLibrary> StaticResult;
  std::optional<MultiArchSharedLibrary> SharedResult;
};

} // namespace clang::ssaf

#endif // LLVM_CLANG_TOOLS_CLANG_SSAF_LINKER_MULTIARCHCREATECLI_H
