//===-- LanguageUtils.cpp - Kernel language utilities ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "LanguageUtils.h"
#include "LanguageRuntime.h"
#include "OffloadAPI.h"

static Error_t convertResult(ol_result_t Result) {
  if (Result == OL_SUCCESS)
    return Success;
  switch (Result->Code) {
  case OL_ERRC_INVALID_VALUE:
  case OL_ERRC_INVALID_ARGUMENT:
  case OL_ERRC_INVALID_NULL_POINTER:
    return ErrorInvalidValue;
  case OL_ERRC_INVALID_DEVICE:
    return ErrorInvalidDevice;
  case OL_ERRC_INVALID_SIZE:
    return ErrorInvalidConfiguration;
  case OL_ERRC_INVALID_NULL_HANDLE:
  case OL_ERRC_INVALID_QUEUE:
  case OL_ERRC_INVALID_EVENT:
  case OL_ERRC_INVALID_CONTEXT:
    return ErrorInvalidResourceHandle;
  default:
    return ErrorUnknown;
  }
}

static thread_local Error_t LastError = Success;

const char *GetErrorName(Error_t Error) {
  switch (Error) {
#define LLVM_OFFLOAD_STRINGIFY_IMPL(NAME) #NAME
#define LLVM_OFFLOAD_STRINGIFY(NAME) LLVM_OFFLOAD_STRINGIFY_IMPL(NAME)
#define LLVM_OFFLOAD_ERR_STR(NAME)                                             \
  case NAME:                                                                   \
    return LLVM_OFFLOAD_STRINGIFY(NAME);
    LLVM_OFFLOAD_ERR_STR(Success)
    LLVM_OFFLOAD_ERR_STR(ErrorInvalidValue)
    LLVM_OFFLOAD_ERR_STR(ErrorInvalidDevice)
    LLVM_OFFLOAD_ERR_STR(ErrorInvalidResourceHandle)
    LLVM_OFFLOAD_ERR_STR(ErrorInvalidConfiguration)
#undef LLVM_OFFLOAD_ERR_STR
#undef LLVM_OFFLOAD_STRINGIFY
#undef LLVM_OFFLOAD_STRINGIFY_IMPL
  default:
    return "Unrecognized error";
  };
}

const char *GetErrorString(Error_t Error) {
  switch (Error) {
  case Success:
    return "No error";
  case ErrorInvalidValue:
    return "Invalid argument value";
  case ErrorInvalidDevice:
    return "Invalid device number";
  case ErrorUnknown:
    return "Unknown error";
  case ErrorInvalidResourceHandle:
    return "Invalid resource handle";
  case ErrorInvalidConfiguration:
    return "Invalid configuration argument";
  }
  return "Unrecognized error";
}

Error_t GetLastError() {
  Error_t Error = LastError;
  LastError = Success;
  return Error;
}

Error_t PeekAtLastError() { return LastError; }

static Error_t getQueueFromStream(Stream_t Stream, ol_queue_handle_t *Queue) {
  if (!Stream)
    return ErrorInvalidValue;
  *Queue = reinterpret_cast<ol_queue_handle_t>(Stream);
  return Success;
}
