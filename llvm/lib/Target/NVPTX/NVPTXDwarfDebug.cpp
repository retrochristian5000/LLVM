//===-- NVPTXDwarfDebug.cpp - NVPTX DwarfDebug Implementation ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements helper functions for NVPTX-specific debug information
// processing.
//
//===----------------------------------------------------------------------===//

#include "NVPTXDwarfDebug.h"
#include "NVPTXSubtarget.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/NVPTXAddrSpace.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

// Command line option to control inlined_at enhancement to lineinfo support.
// Valid only when debuginfo emissionkind is DebugDirectivesOnly or
// LineTablesOnly.
static cl::opt<bool> LineInfoWithInlinedAt(
    "line-info-inlined-at",
    cl::desc("Emit line with inlined_at enhancement for NVPTX"), cl::init(true),
    cl::Hidden);

NVPTXDwarfDebug::NVPTXDwarfDebug(AsmPrinter *A) : DwarfDebug(A) {
  // PTX emits debug strings inline (no .debug_str section), does not support
  // .debug_ranges, and uses sections as references (no temp symbols inside
  // DWARF sections).  DWARF v2 is the default for NVPTX and does not support
  // accelerator tables.
  setUseInlineStrings(true);
  setUseRangesSection(false);
  setUseSectionsAsReferences(true);
  Asm->OutStreamer->getContext().setDwarfVersion(2);
  setTheAccelTableKind(AccelTableKind::None);
}

MCSymbol *NVPTXDwarfDebug::getOrCreateFuncNameSymbol(StringRef LinkageName) {
  return InfoHolder.getStringPool().getEntry(*Asm, LinkageName).getSymbol();
}

bool NVPTXDwarfDebug::isEnhancedLineinfo(const MachineFunction &MF) const {
  const DISubprogram *SP = MF.getFunction().getSubprogram();
  const NVPTXSubtarget &STI = MF.getSubtarget<NVPTXSubtarget>();
  return LineInfoWithInlinedAt && (STI.getPTXVersion() >= 72) && SP &&
         (SP->getUnit()->isDebugDirectivesOnly() ||
          SP->getUnit()->getEmissionKind() == DICompileUnit::LineTablesOnly);
}

/// NVPTX-specific source line recording with inlined_at support.
///
/// Why this exists:
/// NVPTX supports an "enhanced lineinfo" mode where inlining context is carried
/// via line-table directives, rather than full DWARF DIEs. This is conceptually
/// similar to proposals[1] for richer DWARF line tables that carry inline call
/// context and callee identity in the line table. NVPTX implements this via
/// target-specific `.loc` extensions in the PTX ISA[3].
///
/// How it impacts PTX assembly generation:
/// - When enabled (PTX ISA >= 7.2 + line-tables-only / debug-directives-only),
///   we emit multiple consecutive `.loc` directives for a single inlined
///   instruction: the instruction's own location and its `inlined_at` parent
///   chain.
/// - During emission we use `MCStreamer::emitDwarfLocDirectiveWithInlinedAt` to
///   emit an enhanced `.loc` directive[3] that carries the extra
///   `function_name` and `inlined_at` operands in the PTX assembly stream.
///
/// Example (conceptual PTX `.loc` sequence for an inlined callsite):
///   .loc 1 16 3                            // caller location
///   .loc 1  5 3, function_name $L__info_stringN, inlined_at 1 16 3
///                                         // inlined callee location
///   Here, $L__info_stringN is a label (or label+immediate) referring into
///   `.debug_str`.
///
/// How this impacts DWARF :
/// DWARF generation tools that consume this PTX(e.g. ptxas assembler) can use
/// the `inlined_at` and `function_name` operands to extend the DWARF v2
/// line table information.
/// This adds:
/// - a `context` column[2]: the `inlined_at <file> <line> <col>` information
///   populates an inlining "context" (a reference to the parent/callsite row)
///   enabling reconstruction of inline call chains from the line table.
/// - a `function_name` column[2]: the `.loc ... function_name <sym>` identifies
///   the inlined callee associated with a non-zero context.
///
/// References:
/// - [1] DWARF line tables / Two-Level Line Tables:
///   https://wiki.dwarfstd.org/TwoLevelLineTables.md
/// - [2] DWARF issue tracking for Two-Level Line Tables:
///   https://dwarfstd.org/issues/140906.1.html
/// - [3] NVIDIA PTX ISA `.loc` (debugging directives; PTX ISA 7.2+):
///   https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#debugging-directives-loc
void NVPTXDwarfDebug::recordTargetSourceLine(const DebugLoc &DL,
                                             unsigned Flags) {
  // Maintain a work list of .loc to be emitted. If we are emitting the
  // inlined_at directive, we might need to emit additional .loc prior
  // to it for the location contained in the inlined_at.
  SmallVector<const DILocation *, 8> WorkList;
  SmallDenseSet<const DILocation *, 8> WorkListSet;
  const DILocation *EmitLoc = DL.get();

  if (!EmitLoc)
    return;

  const MachineFunction *MF = Asm->MF;
  if (!MF)
    return;

  const bool EnhancedLineinfo = isEnhancedLineinfo(*MF);

  while (EmitLoc) {
    // Get the scope for the current location.
    const DIScope *Scope = EmitLoc->getScope();
    if (!Scope)
      break; // scope is null, we are done.

    // Check if this loc is already in work list, if so, we are done.
    if (WorkListSet.contains(EmitLoc))
      break;

    // Add this location to the work list.
    WorkList.push_back(EmitLoc);
    WorkListSet.insert(EmitLoc);

    if (!EnhancedLineinfo) // No enhanced lineinfo, we are done.
      break;

    const DILocation *IA = EmitLoc->getInlinedAt();
    // Check if this has inlined_at information, and if the parent location
    // has not yet been emitted. If already emitted, we don't need to
    // re-emit the parent chain.
    if (IA && !EmittedInlinedAtLocs.contains(IA))
      EmitLoc = IA;
    else // We are done.
      break;
  }

  const unsigned CUID = Asm->OutStreamer->getContext().getDwarfCompileUnitID();
  // Traverse the work list, and emit .loc.
  while (!WorkList.empty()) {
    const DILocation *Current = WorkList.pop_back_val();
    const DIScope *Scope = Current->getScope();

    if (!Scope)
      llvm_unreachable("we shouldn't be here for null scope");

    const DILocation *InlinedAt = Current->getInlinedAt();
    StringRef Fn = Scope->getFilename();
    const unsigned Line = Current->getLine();
    const unsigned Col = Current->getColumn();
    unsigned Discriminator = 0;
    if (Line != 0 && getDwarfVersion() >= 4)
      if (const DILexicalBlockFile *LBF = dyn_cast<DILexicalBlockFile>(Scope))
        Discriminator = LBF->getDiscriminator();

    const unsigned FileNo = static_cast<DwarfCompileUnit &>(*getUnits()[CUID])
                                .getOrCreateSourceID(Scope->getFile());

    if (EnhancedLineinfo && InlinedAt) {
      const unsigned FileIA = static_cast<DwarfCompileUnit &>(*getUnits()[CUID])
                                  .getOrCreateSourceID(InlinedAt->getFile());
      const DISubprogram *SubProgram = getDISubprogram(Current->getScope());
      DwarfStringPoolEntryRef Entry = InfoHolder.getStringPool().getEntry(
          *Asm, SubProgram->getLinkageName());
      Asm->OutStreamer->emitDwarfLocDirectiveWithInlinedAt(
          FileNo, Line, Col, FileIA, InlinedAt->getLine(),
          InlinedAt->getColumn(), Entry.getSymbol(), Flags, 0, Discriminator,
          Fn);
    } else {
      Asm->OutStreamer->emitDwarfLocDirective(FileNo, Line, Col, Flags, 0,
                                              Discriminator, Fn);
    }
    // Mark this location as emitted so we don't re-emit the parent chain
    // for subsequent instructions that share the same inlined_at parent.
    if (EnhancedLineinfo)
      EmittedInlinedAtLocs.insert(Current);
  }

  recordIntermediateLoc(DL, Flags);
}

void NVPTXDwarfDebug::recordIntermediateLoc(const DebugLoc &DL,
                                            unsigned Flags) {
  // Intermediate-IR layers live on the frame that was outermost when the tile
  // snapshot was taken; after later inlining that frame can sit anywhere in the
  // inlined-at chain. Walk outward from the head and use the first (innermost)
  // frame that carries layers: it is the most specific intermediate origin for
  // this PC -- the instruction actually being executed -- and for layer-less
  // inlined code it is the nearest enclosing frame (continuous coverage).
  const DILocation *Loc = DL.get();
  while (Loc && !Loc->getRawIRLayers())
    Loc = Loc->getInlinedAt();
  if (!Loc)
    return;
  DILayerLocList *Layers = Loc->getIRLayers();
  if (!Layers)
    return;
  // Emit a secondary .loc_intermediate DWARF directive for each intermediate-IR
  // layer. Each DILayerLoc carries its file/line/column directly (no scope and
  // no discriminator).
  const unsigned CUID = Asm->OutStreamer->getContext().getDwarfCompileUnitID();
  for (unsigned I = 0, E = Layers->getNumLayers(); I != E; ++I) {
    // The verifier guarantees each entry is a non-null DILayerLoc whose file is
    // a non-null DIFile carrying a checksum, so no null checks are needed here.
    const DILayerLoc *L = Layers->getLayer(I);
    const unsigned Line = L->getLine();
    const DIFile *OrigFile = L->getFile();
    // Skip the DWARF "no line" sentinel (line 0 is valid IR, not a verifier
    // invariant).
    if (!Line)
      continue;
    // Secondary .file name: a file that carries source uses its checksum digest
    // (a stable per-content token that pairs with the .code_block's .source
    // blob); a source-less file has nothing to content-address, so it uses its
    // filename instead.
    const StringRef SecondaryFilename = OrigFile->getSource()
                                            ? OrigFile->getChecksum()->Value
                                            : OrigFile->getFilename();
    DIFile *SecondaryFile = DIFile::get(Loc->getContext(), SecondaryFilename,
                                        OrigFile->getDirectory());
    const unsigned FileNo = static_cast<DwarfCompileUnit &>(*getUnits()[CUID])
                                .getOrCreateSourceID(SecondaryFile);
    const unsigned Col = L->getColumn();
    Asm->OutStreamer->emitDwarfLocDirective(
        FileNo, Line, Col, Flags, 0, /*Discriminator=*/0,
        OrigFile->getFilename(), "", ".loc_intermediate");
    // First encounter wins: a given intermediate file's number and kind are
    // invariant (getOrCreateSourceID is memoized; kind uniformity is a verifier
    // invariant), so re-inserting on every layered instruction is wasted work.
    IntermediateFiles.insert({OrigFile, {FileNo, L->getKind()}});
  }
}

std::string NVPTXDwarfDebug::buildIntermediateSourceSection(Module &M) {
  if (IntermediateFiles.empty())
    return {};

  // Build the body first so an empty section header/footer isn't emitted when
  // no file produced a .code_block (e.g. every intermediate file lacks source).
  // MapVector iterates in first-reference (insertion) order, which is ascending
  // .file-number order -- deterministic for a given compilation and consistent
  // with the emitted .file directives, so no sort is needed.
  std::string Body;
  raw_string_ostream BodyOS(Body);
  for (const auto &[F, Info] : IntermediateFiles) {
    // Source now lives on DIFile.source; a null source omits the .code_block.
    std::optional<StringRef> Source = F->getSource();
    if (!Source)
      continue;
    BodyOS << "  .code_block {\n";
    BodyOS << "    .ir_name: \"" << Info.Kind << "\"\n";
    BodyOS << "    .sourceFileName: " << Info.FileNum << "\n";
    BodyOS << "    .source: <<< " << *Source << " >>>\n";
    BodyOS << "  }\n";
  }

  if (Body.empty())
    return {};

  std::string Buf;
  raw_string_ostream OS(Buf);
  OS << ".nv_intermediate_source_section {\n" << Body << "}";
  return Buf;
}

/// NVPTX-specific debug info initialization.
void NVPTXDwarfDebug::initializeTargetDebugInfo(const MachineFunction &MF) {
  EmittedInlinedAtLocs.clear();
}

// PTX does not support subtracting labels from the code section in the
// debug_loc section.  To work around this, the NVPTX backend needs the
// compile unit to have no low_pc in order to have a zero base_address
// when handling debug_loc in cuda-gdb.
bool NVPTXDwarfDebug::shouldAttachCompileUnitRanges() const {
  return !tuneForGDB();
}

// Same label-subtraction limitation as above: cuda-gdb doesn't handle
// setting a per-variable base to zero, so we emit labels with no base
// while having no compile unit low_pc.
bool NVPTXDwarfDebug::shouldResetBaseAddress(const MCSection &Section) const {
  return tuneForGDB();
}

static unsigned translateToNVVMDWARFAddrSpace(unsigned AddrSpace) {
  switch (AddrSpace) {
  case NVPTXAS::ADDRESS_SPACE_GENERIC:
    return NVPTXAS::DWARF_ADDR_generic_space;
  case NVPTXAS::ADDRESS_SPACE_GLOBAL:
    return NVPTXAS::DWARF_ADDR_global_space;
  case NVPTXAS::ADDRESS_SPACE_SHARED:
    return NVPTXAS::DWARF_ADDR_shared_space;
  case NVPTXAS::ADDRESS_SPACE_CONST:
    return NVPTXAS::DWARF_ADDR_const_space;
  case NVPTXAS::ADDRESS_SPACE_LOCAL:
    return NVPTXAS::DWARF_ADDR_local_space;
  default:
    llvm_unreachable(
        "Cannot translate unknown address space to DWARF address space");
    return AddrSpace;
  }
}

// cuda-gdb requires DW_AT_address_class on variable DIEs.  The address space
// is encoded in the DIExpression as a DW_OP_constu <DWARF Address Space>
// DW_OP_swap DW_OP_xderef sequence.  We strip that sequence from the
// expression and return the address space so the caller can emit
// DW_AT_address_class separately.
const DIExpression *NVPTXDwarfDebug::adjustExpressionForTarget(
    const DIExpression *Expr, std::optional<unsigned> &TargetAddrSpace) const {
  if (!tuneForGDB())
    return Expr;
  unsigned LocalAddrSpace;
  const DIExpression *NewExpr =
      DIExpression::extractAddressClass(Expr, LocalAddrSpace);
  if (NewExpr != Expr) {
    TargetAddrSpace = LocalAddrSpace;
    return NewExpr;
  }
  return Expr;
}

// Emit DW_AT_address_class for cuda-gdb.  See NVPTXAS::DWARF_AddressSpace.
//
// The address class depends on the variable's storage kind:
//   Global:     from the expression (if encoded) or the IR address space
//   Register:   DWARF_ADDR_reg_space (no expression means register location)
//   FrameIndex: from the expression (if encoded) or DWARF_ADDR_local_space
void NVPTXDwarfDebug::addTargetVariableAttributes(
    DwarfCompileUnit &CU, DIE &Die, std::optional<unsigned> TargetAddrSpace,
    VariableLocationKind VarLocKind, const GlobalVariable *GV) const {
  if (!tuneForGDB())
    return;

  unsigned DefaultAddrSpace = NVPTXAS::DWARF_ADDR_global_space;
  switch (VarLocKind) {
  case VariableLocationKind::Global:
    if (!TargetAddrSpace && GV)
      TargetAddrSpace =
          translateToNVVMDWARFAddrSpace(GV->getType()->getAddressSpace());
    DefaultAddrSpace = NVPTXAS::DWARF_ADDR_global_space;
    break;
  case VariableLocationKind::Register:
    DefaultAddrSpace = NVPTXAS::DWARF_ADDR_reg_space;
    break;
  case VariableLocationKind::FrameIndex:
    DefaultAddrSpace = NVPTXAS::DWARF_ADDR_local_space;
    break;
  }

  CU.addUInt(Die, dwarf::DW_AT_address_class, dwarf::DW_FORM_data1,
             TargetAddrSpace.value_or(DefaultAddrSpace));
}

void NVPTXDwarfDebug::finishTargetUnitAttributes(const DICompileUnit &DIUnit,
                                                 DwarfCompileUnit &NewCU) {
  uint16_t Dialect = DIUnit.getSourceLanguage().getDialect();
  // A zero dialect means "no dialect specified"; nothing to emit. This field
  // should have already been range-checked by the assembly parser, IR verifier,
  // and bitcode reader.
  assert(Dialect <= dwarf::DW_LLVM_LANG_DIALECT_max &&
         "Invalid or unsupported NVPTX language dialect.");
  if (Dialect == 0)
    return;

  NewCU.addUInt(NewCU.getUnitDie(), dwarf::DW_AT_LLVM_language_dialect,
                dwarf::DW_FORM_data1, Dialect);
}
