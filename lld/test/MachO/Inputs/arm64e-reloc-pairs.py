#!/usr/bin/env python3
"""Create malformed ARM64e relocation pairs from one valid llvm-mc object."""

import pathlib
import struct
import sys

source = bytearray(pathlib.Path(sys.argv[1]).read_bytes())
dest = pathlib.Path(sys.argv[2])


def u32(offset):
    return struct.unpack_from("<I", source, offset)[0]


if u32(0) != 0xFEEDFACF:
    raise SystemExit("expected little-endian 64-bit Mach-O")
relocation = None
count_field = None
end = 32 + u32(20)
position = 32
if end > len(source):
    raise SystemExit("invalid Mach-O load commands")
for _ in range(u32(16)):
    cmd, size = struct.unpack_from("<II", source, position)
    if size < 8 or position + size > end:
        raise SystemExit("invalid load-command size")
    if cmd == 0x19:  # LC_SEGMENT_64
        for section_index in range(u32(position + 64)):
            off = position + 72 + section_index * 80
            if off + 80 > position + size:
                raise SystemExit("invalid section header")
            if source[off:off + 6] != b"__data":
                continue
            if u32(off + 60) != 2:
                raise SystemExit("expected two authenticated relocations")
            relocation = u32(off + 56)
            count_field = off + 60
            break
    position += size
if relocation is None or relocation + 16 > len(source):
    raise SystemExit("missing authenticated relocation table")
for i in (0, 1):
    word = u32(relocation + 8 * i + 4)
    if word >> 28 != 11:  # ARM64_RELOC_AUTHENTICATED_POINTER
        raise SystemExit("fixture must begin with authenticated pointers")

for name, kind, shorten in (
    ("missing-addend", 10, True),
    ("auth-after-addend", 10, False),
    ("missing-subtractor", 1, True),
    ("auth-after-subtractor", 1, False),
):
    result = bytearray(source)
    if shorten:
        struct.pack_into("<I", result, count_field, 1)
    word = u32(relocation + 4)
    struct.pack_into("<I", result, relocation + 4,
                     (word & 0x0FFFFFFF) | (kind << 28))
    (dest / (name + ".o")).write_bytes(result)
