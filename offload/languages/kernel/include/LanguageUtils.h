//===-- LanguageUtils.h - Kernel Language utility functions ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#ifndef LANGUAGE
#error This file should be included, or used, with a LANGUAGE macro set.
#endif

#include "OffloadAPI.h"

/// Convert an ol_result_t to the active language's Error_t.
static Error_t convertResult(ol_result_t Result);

/// Convert a Stream_t to an ol_queue_handle_t.
static Error_t getQueueFromStream(Stream_t Stream, ol_queue_handle_t *Queue);
