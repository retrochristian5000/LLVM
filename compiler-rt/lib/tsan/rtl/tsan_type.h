//===-- tsan_type.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TSAN_TYPE_H
#define TSAN_TYPE_H

#include "tsan_report.h"

namespace __tsan {

// Classify a race using the current and previous access flags. The ordering
// mirrors the existing vptr/use-after-free precedence in ReportRace.
ReportType RaceReportType(AccessType current, AccessType previous);

#if !SANITIZER_GO
// Human-readable warning heading for a classified report.
const char *ReportTypeString(ReportType type, uptr external_tag);
#endif

}  // namespace __tsan

#endif  // TSAN_TYPE_H
