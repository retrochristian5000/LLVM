//===-- AArch64SchedAppleA18Pro.td - A18 Pro scheduling baseline -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Distinguish A18 Pro from the Apple A7-era Cyclone instruction/resource
// schedule. We do not yet have measured A18 Pro instruction latencies, issue
// ports or reorder-buffer capacity, so do not import Cyclone's per-opcode
// WriteRes, ProcResource or SchedWriteVariant records.
//
// The coarse parameters are explicitly *temporary* values carried over from
// Cyclone to avoid changing all scheduler heuristics at the same time. They
// are NOT measurements of A18 Pro. CompleteModel=0 is required until A18 Pro
// per-opcode schedules are populated from measurements or Apple's CPU guide.
// Do not list SME/SME2 as unsupported: the CPU feature profile enables them.
//
// A18 Pro and A18 are distinct CPU families; changes here must be based on
// A18 Pro measurements rather than the M4 or non-Pro A18 marketing label.
//
//===----------------------------------------------------------------------===//

def AppleA18ProModel : SchedMachineModel {
  let IssueWidth = 6;           // Provisional Cyclone-derived baseline.
  let MicroOpBufferSize = 192;  // Provisional; not the A18 Pro ROB size.
  let LoadLatency = 4;          // Provisional; awaiting measurements.
  let MispredictPenalty = 16;   // Provisional; awaiting measurements.
  let CompleteModel = 0;
}
