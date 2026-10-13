#!/usr/bin/env python3
"""Verify on-disk ARM64e PAC metadata in linked Mach-O chained pointers."""

import struct
import sys
from pathlib import Path

image = Path(sys.argv[1]).read_bytes()
def u32(offset):
    return struct.unpack_from("<I", image, offset)[0]

def u64(offset):
    return struct.unpack_from("<Q", image, offset)[0]

assert u32(0) == 0xfeedfacf, "expected 64-bit little-endian Mach-O"
assert u32(8) & 0xff == 2, "expected ARM64e CPU subtype"
sections = {}
fixup_command = None
position = 32
for _ in range(u32(16)):  # mach_header_64.ncmds
    command, command_size = struct.unpack_from("<II", image, position)
    assert command_size >= 8
    if command == 0x19:  # LC_SEGMENT_64
        segment_name = image[position + 8 : position + 24].split(b"\0", 1)[0]
        for i in range(u32(position + 64)):
            section = position + 72 + i * 80
            section_name = image[section : section + 16].split(b"\0", 1)[0]
            sections[(segment_name, section_name)] = u32(section + 48)
    elif command == 0x80000034:  # LC_DYLD_CHAINED_FIXUPS
        assert fixup_command is None
        fixup_command = u32(position + 8)
    position += command_size

assert fixup_command is not None, "missing LC_DYLD_CHAINED_FIXUPS"
start_offset = u32(fixup_command + 4)
starts = fixup_command + start_offset
segment_count = u32(starts)
formats = []
for i in range(segment_count):
    segment_offset = u32(starts + 4 + i * 4)
    if segment_offset:
        segment_info = starts + segment_offset
        formats.append(struct.unpack_from("<H", image, segment_info + 6)[0])
assert 12 in formats, "ARM64e fixups must use USERLAND24"

data = sections[(b"__DATA", b"__data")]
for index in range(8):
    raw = u64(data + index * 8)
    expected_key = index % 4
    expected_diversity = (42, 513, 7, 65535)[expected_key]
    expected_address_diversity = (1, 0, 1, 0)[expected_key]
    expected_next = 1 if index != 7 else 0
    assert raw >> 63 == 1, f"slot {index}: lost PAC authentication"
    assert (raw >> 62) & 1 == int(index >= 4), f"slot {index}: bind/rebase"
    assert (raw >> 49) & 3 == expected_key, f"slot {index}: PAC key"
    assert (raw >> 48) & 1 == expected_address_diversity, (
        f"slot {index}: address discriminator")
    assert (raw >> 32) & 0xffff == expected_diversity, (
        f"slot {index}: diversity")
    assert (raw >> 51) & 0x7ff == expected_next, (
        f"slot {index}: chain stride")
    if index >= 4:
        assert raw & 0xffffff == 0, f"slot {index}: wrong import ordinal"
print("ARM64e rebase/bind PAC metadata: verified")
