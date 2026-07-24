//===- MultiArchCreateCLI.cpp ---------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  Implements the `multi-arch create` CLI action. The class contains no
//  cl::opt globals of its own: the driver in SSAFLinker.cpp owns all flag
//  definitions and hands values in via the Config struct passed to run().
//
//===----------------------------------------------------------------------===//

#include "MultiArchCreateCLI.h"

#include "clang/ScalableStaticAnalysis/Core/EntityLinker/LUSummaryEncoding.h"
#include "clang/ScalableStaticAnalysis/Core/EntityLinker/StaticLibrary.h"
#include "clang/ScalableStaticAnalysis/Core/EntityLinker/TUSummaryEncoding.h"
#include "clang/ScalableStaticAnalysis/Core/Model/BuildNamespace.h"
#include "clang/ScalableStaticAnalysis/Core/Serialization/SerializationFormat.h"
#include "clang/ScalableStaticAnalysis/Core/Support/ErrorBuilder.h"
#include "clang/ScalableStaticAnalysis/Core/Support/FormatProviders.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Path.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace llvm;
using namespace clang::ssaf;

namespace path = llvm::sys::path;

//===----------------------------------------------------------------------===//
// Error Messages / Local Types
//===----------------------------------------------------------------------===//

namespace {

constexpr const char *ReadingArtifact = "Reading artifact '{0}'";

constexpr const char *NoInputs =
    "no input artifacts: at least one input is required";

constexpr const char *InvalidInputKind =
    "'{0}' is a raw TU summary, not a valid input to multi-arch create: run "
    "static-library create or an entity-linking step first";

constexpr const char *MixedFamily =
    "input '{0}' is a {1} artifact, but a preceding input established this "
    "bundle as {2}";

constexpr const char *NamespaceFlagNotSupportedForShared =
    "--namespace is not supported when bundling shared-library members: "
    "identity is inferred from the first member's own namespace";

constexpr const char *NamespaceMismatch =
    "namespace {0} from '{1}' does not match expected namespace {2}";

constexpr const char *NoCandidateMembers =
    "no candidate members could be derived from the given inputs: at least "
    "one member is required";

constexpr const char *DuplicateTriple =
    "duplicate architecture slice '{0}' contributed by both '{1}' and '{2}'";

constexpr const char *StaticFamilyName = "static-library";
constexpr const char *SharedFamilyName = "shared-library";

/// One StaticLibrary destined for the output MultiArchStaticLibrary, tagged
/// with the file it came from (for diagnostics).
struct StaticCandidate {
  std::unique_ptr<StaticLibrary> Member;
  std::string SourceFile;
};

/// One LUSummaryEncoding destined for the output MultiArchSharedLibrary,
/// tagged with the file it came from (for diagnostics).
struct SharedCandidate {
  std::unique_ptr<LUSummaryEncoding> Member;
  std::string SourceFile;
};

/// A namespace observed on a static-family input, tagged with whether it
/// came from a wrapper (fat bundle) namespace or a bare-member namespace,
/// and which file it came from.
struct StaticWitness {
  BuildNamespace NS;
  bool IsWrapper;
  std::string SourceFile;
};

/// A namespace observed on a shared-family input, tagged with which file it
/// came from. Unlike the static family, there is no separate wrapper-kind
/// tag: a MultiArchSharedLibrary's own Namespace and a LUSummaryEncoding's
/// own LUNamespace are compared for exact equality against the same target.
struct SharedWitness {
  NestedBuildNamespace NS;
  std::string SourceFile;
};

} // namespace

//===----------------------------------------------------------------------===//
// MultiArchCreateCLI
//===----------------------------------------------------------------------===//

template <typename ResultT, typename CandidateT>
void MultiArchCreateCLI::insertCandidates(ResultT &Result,
                                          std::vector<CandidateT> &Candidates) {
  std::vector<std::pair<std::string, std::string>> AcceptedTriples;
  for (auto &Candidate : Candidates) {
    std::string NormalizedTriple =
        llvm::Triple::normalize(Candidate.Member->TargetTriple.str());
    auto [It, Inserted] = Result.Members.insert(std::move(Candidate.Member));
    if (!Inserted) {
      auto Existing = std::find_if(
          AcceptedTriples.begin(), AcceptedTriples.end(),
          [&](const auto &P) { return P.first == NormalizedTriple; });
      fail(DuplicateTriple, NormalizedTriple, Existing->second,
           Candidate.SourceFile);
    }
    AcceptedTriples.emplace_back(std::move(NormalizedTriple),
                                 Candidate.SourceFile);
  }
}

void MultiArchCreateCLI::run(llvm::TimerGroup &TG, const Config &InCfg) {
  Cfg = InCfg;

  info(0, "Creating multi-arch bundle started.");

  {
    info(1, "Validating input.");
    validate(TG);
  }

  info(1, "Bundling members.");
  bundle(TG);
  write(TG);

  info(0, "Creating multi-arch bundle finished.");
}

void MultiArchCreateCLI::validate(llvm::TimerGroup &TG) {
  llvm::Timer TValidate("validate", "Validate Input", TG);
  llvm::TimeRegion _(Cfg.Time ? &TValidate : nullptr);

  OutputFile = FormatFile::fromOutputPath(Cfg.OutputPath);
  info(2, "Validated output path '{0}'.", OutputFile.Path);

  if (Cfg.InputPaths.empty()) {
    fail(NoInputs);
  }
  for (const auto &InputPath : Cfg.InputPaths) {
    InputFiles.push_back(FormatFile::fromInputPath(InputPath));
  }
  info(2, "Validated {0} input artifact paths.", InputFiles.size());
}

void MultiArchCreateCLI::bundle(llvm::TimerGroup &TG) {
  llvm::Timer TRead("read", "Read Artifacts", TG);
  llvm::Timer TAssemble("assemble", "Flatten/Assemble Multi-Arch Bundle", TG);

  std::vector<StaticCandidate> StaticCandidates;
  std::vector<SharedCandidate> SharedCandidates;
  std::vector<StaticWitness> StaticWitnesses;
  std::vector<SharedWitness> SharedWitnesses;

  info(2, "Classifying inputs.");

  for (auto [Index, InputFile] : llvm::enumerate(InputFiles)) {
    info(3, "[{0}/{1}] Reading '{2}'.", (Index + 1), InputFiles.size(),
         InputFile.Path);

    std::optional<ArtifactEncoding> MaybeEncoding;
    {
      llvm::TimeRegion _(Cfg.Time ? &TRead : nullptr);

      auto ExpectedEncoding =
          InputFile.Format->readArtifactEncoding(InputFile.Path);
      if (!ExpectedEncoding) {
        fail(ErrorBuilder::wrap(ExpectedEncoding.takeError())
                 .context(ReadingArtifact, InputFile.Path)
                 .build());
      }
      MaybeEncoding.emplace(std::move(*ExpectedEncoding));
    }
    ArtifactEncoding &Encoding = *MaybeEncoding;

    llvm::TimeRegion _(Cfg.Time ? &TAssemble : nullptr);

    info(3, "[{0}/{1}] Classifying '{2}'.", (Index + 1), InputFiles.size(),
         InputFile.Path);

    if (std::holds_alternative<TUSummaryEncoding>(Encoding)) {
      fail(InvalidInputKind, InputFile.Path);
    } else if (auto *SL = std::get_if<StaticLibrary>(&Encoding)) {
      if (ResultFamily == Family::Unknown) {
        ResultFamily = Family::Static;
      } else if (ResultFamily != Family::Static) {
        fail(MixedFamily, InputFile.Path, StaticFamilyName, SharedFamilyName);
      }
      StaticWitnesses.push_back(
          {SL->Namespace, /*IsWrapper=*/false, InputFile.Path});
      StaticCandidates.push_back(
          {std::make_unique<StaticLibrary>(std::move(*SL)), InputFile.Path});
    } else if (auto *MASL = std::get_if<MultiArchStaticLibrary>(&Encoding)) {
      if (ResultFamily == Family::Unknown) {
        ResultFamily = Family::Static;
      } else if (ResultFamily != Family::Static) {
        fail(MixedFamily, InputFile.Path, StaticFamilyName, SharedFamilyName);
      }
      StaticWitnesses.push_back(
          {MASL->Namespace, /*IsWrapper=*/true, InputFile.Path});
      while (!MASL->Members.empty()) {
        auto Node = MASL->Members.extract(MASL->Members.begin());
        StaticCandidates.push_back({std::move(Node.value()), InputFile.Path});
      }
    } else if (auto *LU = std::get_if<LUSummaryEncoding>(&Encoding)) {
      if (ResultFamily == Family::Unknown) {
        ResultFamily = Family::Shared;
      } else if (ResultFamily != Family::Shared) {
        fail(MixedFamily, InputFile.Path, SharedFamilyName, StaticFamilyName);
      }
      SharedWitnesses.push_back({LU->LUNamespace, InputFile.Path});
      SharedCandidates.push_back(
          {std::make_unique<LUSummaryEncoding>(std::move(*LU)),
           InputFile.Path});
    } else if (auto *MASharedL =
                   std::get_if<MultiArchSharedLibrary>(&Encoding)) {
      if (ResultFamily == Family::Unknown) {
        ResultFamily = Family::Shared;
      } else if (ResultFamily != Family::Shared) {
        fail(MixedFamily, InputFile.Path, SharedFamilyName, StaticFamilyName);
      }
      SharedWitnesses.push_back({MASharedL->Namespace, InputFile.Path});
      while (!MASharedL->Members.empty()) {
        auto Node = MASharedL->Members.extract(MASharedL->Members.begin());
        SharedCandidates.push_back({std::move(Node.value()), InputFile.Path});
      }
    }
  }

  if (ResultFamily == Family::Shared && !Cfg.Namespace.empty()) {
    fail(NamespaceFlagNotSupportedForShared);
  }

  if (ResultFamily == Family::Static) {
    std::string TargetName = Cfg.Namespace.empty()
                                 ? path::stem(OutputFile.Path).str()
                                 : Cfg.Namespace.str();
    BuildNamespace ExpectedBare(BuildNamespaceKind::StaticLibrary, TargetName);
    BuildNamespace ExpectedWrapper(BuildNamespaceKind::MultiArchStaticLibrary,
                                   TargetName);
    info(2, "Target namespace: '{0}'.", ExpectedWrapper);

    for (const auto &Witness : StaticWitnesses) {
      const BuildNamespace &Expected =
          Witness.IsWrapper ? ExpectedWrapper : ExpectedBare;
      if (Witness.NS != Expected) {
        fail(NamespaceMismatch, Witness.NS, Witness.SourceFile, Expected);
      }
    }

    if (StaticCandidates.empty()) {
      fail(NoCandidateMembers);
    }

    StaticResult.emplace(ExpectedWrapper);
    insertCandidates(*StaticResult, StaticCandidates);

    info(2, "Bundled {0} architecture slice(s).", StaticResult->Members.size());
  } else {
    assert(ResultFamily == Family::Shared &&
           "validate() guarantees at least one input; every input either "
           "sets Family or fails the process");

    NestedBuildNamespace TargetNamespace = SharedWitnesses.front().NS;
    info(2, "Target namespace: '{0}'.", TargetNamespace);

    for (const auto &Witness : SharedWitnesses) {
      if (Witness.NS != TargetNamespace) {
        fail(NamespaceMismatch, Witness.NS, Witness.SourceFile,
             TargetNamespace);
      }
    }

    if (SharedCandidates.empty()) {
      fail(NoCandidateMembers);
    }

    SharedResult.emplace(TargetNamespace);
    insertCandidates(*SharedResult, SharedCandidates);

    info(2, "Bundled {0} architecture slice(s).", SharedResult->Members.size());
  }
}

void MultiArchCreateCLI::write(llvm::TimerGroup &TG) {
  info(2, "Writing multi-arch bundle to '{0}'.", OutputFile.Path);

  llvm::Timer TWrite("write", "Write Multi-Arch Bundle", TG);
  llvm::TimeRegion _(Cfg.Time ? &TWrite : nullptr);

  llvm::Error Err =
      ResultFamily == Family::Static
          ? OutputFile.Format->writeArtifactEncoding(
                ArtifactEncoding(std::move(*StaticResult)), OutputFile.Path)
          : OutputFile.Format->writeArtifactEncoding(
                ArtifactEncoding(std::move(*SharedResult)), OutputFile.Path);
  if (Err) {
    fail(std::move(Err));
  }
}
