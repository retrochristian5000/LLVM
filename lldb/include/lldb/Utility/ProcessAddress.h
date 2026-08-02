//===-- ProcessAddress.h ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_UTILITY_PROCESSADDRESS_H
#define LLDB_UTILITY_PROCESSADDRESS_H

#include "lldb/lldb-defines.h"
#include "lldb/lldb-types.h"

namespace lldb_private {

/// An address in a process, qualified by an address space.
///
/// The address space is a numeric id reported by the process (see
/// Process::GetAddressSpaces). LLDB_DEFAULT_ADDRESS_SPACE_ID is the default
/// (flat) address space, so a ProcessAddress with no space behaves like a plain
/// lldb::addr_t.
class ProcessAddress {
  lldb::addr_t m_value;
  uint64_t m_addr_space = LLDB_DEFAULT_ADDRESS_SPACE_ID;

public:
  /// Implicit so existing lldb::addr_t call sites keep working.
  ProcessAddress(lldb::addr_t load_addr) : m_value(load_addr) {}

  ProcessAddress(lldb::addr_t addr, uint64_t addr_space)
      : m_value(addr), m_addr_space(addr_space) {}

  bool IsInDefaultAddressSpace() const {
    return m_addr_space == LLDB_DEFAULT_ADDRESS_SPACE_ID;
  }

  lldb::addr_t GetValue() const { return m_value; }

  uint64_t GetAddressSpace() const { return m_addr_space; }
};

} // namespace lldb_private

#endif // LLDB_UTILITY_PROCESSADDRESS_H
