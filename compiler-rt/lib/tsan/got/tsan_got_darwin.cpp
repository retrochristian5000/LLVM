//===-- tsan_got_darwin.cpp ----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Darwin Mach-O GOT-equivalent symbol-pointer metadata. This is read-only:
// dyld owns binding and pointer authentication (including arm64e).
// The TSan interface and AArch64 relocation check remain in interface/.
//
//===----------------------------------------------------------------------===//

#include "../interface/tsan_interface_got.h"

#if SANITIZER_APPLE

#include "../endian/tsan_endian_darwin.h"
#include "../rtl/tsan_macho.h"

namespace __tsan {

using __sanitizer::uptr;

bool GetMainExecutableMachOGOTSlots(MachOGOTSlots *slots) {
  if (!slots)
    return false;
  slots->lazy = 0;
  slots->non_lazy = 0;

  const mach_header *image = MachOExecutableHeader();
  // Dyld supplies native-endian, in-memory images. Never interpret swapped
  // load commands as native pointer-bearing section data.
  if (MachO64ByteOrder(image) != MachOByteOrder::kNative)
    return false;

  const auto *header = reinterpret_cast<const mach_header_64 *>(image);
  const uptr image_address = reinterpret_cast<uptr>(image);
  const uptr max_address = ~static_cast<uptr>(0);
  if (image_address > max_address - sizeof(mach_header_64))
    return false;

  uptr command_address = image_address + sizeof(mach_header_64);
  if (header->sizeofcmds > max_address - command_address)
    return false;
  const uptr command_end = command_address + header->sizeofcmds;
  // Every load command occupies at least a load_command header.
  if (header->ncmds > header->sizeofcmds / sizeof(load_command))
    return false;

  for (unsigned i = 0; i < header->ncmds; ++i) {
    if (command_address > command_end ||
        command_end - command_address < sizeof(load_command))
      return false;

    const auto *command =
        reinterpret_cast<const load_command *>(command_address);
    if (command->cmdsize < sizeof(load_command) ||
        command->cmdsize > command_end - command_address)
      return false;

    if (command->cmd == LC_SEGMENT_64) {
      if (command->cmdsize < sizeof(segment_command_64))
        return false;
      const auto *segment =
          reinterpret_cast<const segment_command_64 *>(command);
      const uptr section_bytes =
          command->cmdsize - sizeof(segment_command_64);
      if (segment->nsects > section_bytes / sizeof(section_64))
        return false;

      const auto *sections =
          reinterpret_cast<const section_64 *>(segment + 1);
      for (unsigned j = 0; j < segment->nsects; ++j) {
        const section_64 &section = sections[j];
        const unsigned kind = section.flags & SECTION_TYPE;
        uptr *counter = nullptr;
        if (kind == S_NON_LAZY_SYMBOL_POINTERS)
          counter = &slots->non_lazy;
        else if (kind == S_LAZY_SYMBOL_POINTERS ||
                 kind == S_LAZY_DYLIB_SYMBOL_POINTERS)
          counter = &slots->lazy;
        if (!counter)
          continue;

        // Count pointer-sized entries, not bytes. A partial pointer is not
        // a valid symbol-pointer section and must not be rounded up.
        if (section.size % sizeof(uptr))
          return false;
        const uptr entries = section.size / sizeof(uptr);
        if (entries > max_address - *counter)
          return false;
        *counter += entries;
      }
    }
    command_address += command->cmdsize;
  }
  return true;
}


}  // namespace __tsan

#endif  // SANITIZER_APPLE
