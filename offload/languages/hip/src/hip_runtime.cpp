/*===---- hip_runtime.cpp - HIP runtime api implementations ----------------===
 *
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *===-----------------------------------------------------------------------===
 */

#include "hip_runtime.h"

#include "LanguageLaunch.h"
#include "OffloadAPI.h"

#define LANGUAGE hip

#include "../../kernel/src/LanguageRuntime.cpp"
