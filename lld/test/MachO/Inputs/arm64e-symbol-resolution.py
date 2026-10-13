#!/usr/bin/env python3
"""Audit actual dyld chained import ordinals and PAC metadata in an arm64e bundle."""

from pathlib import Path
import struct
import sys

image = Path(sys.argv[1]).read_bytes()


def u32(off):
    return struct.unpack_from("<I", image, off)[0]


def u64(off):
    return struct.unpack_from("<Q", image, off)[0]


def sign(value, bits):
    top = 1 << (bits - 1)
    return (value ^ top) - top


assert len(image) >= 32 and u32(0) == 0xFEEDFACF
assert u32(4) == 0x0100000C, "not an ARM64 Mach-O image"
assert u32(8) == 0x80000002, "not versioned user arm64e ABI 0"
assert u32(12) == 8, "expected MH_BUNDLE, not MH_DYLIB"

fixup_off = None
data_off = None
end = 32 + u32(20)
assert end <= len(image), "invalid Mach-O load commands"
pos = 32
for _ in range(u32(16)):
    assert pos + 8 <= end
    command, size = struct.unpack_from("<II", image, pos)
    assert size >= 8 and pos + size <= end
    if command == 0x80000034:  # LC_DYLD_CHAINED_FIXUPS
        assert fixup_off is None
        fixup_off = u32(pos + 8)
    elif command == 0x19:  # LC_SEGMENT_64
        count = u32(pos + 64)
        assert pos + 72 + 80 * count <= pos + size
        for i in range(count):
            sec = pos + 72 + 80 * i
            name = image[sec:sec + 16].split(b"\0", 1)[0]
            seg = image[sec + 16:sec + 32].split(b"\0", 1)[0]
            if (seg, name) == (b"__DATA", b"__data"):
                assert data_off is None
                data_off = u32(sec + 48)
    pos += size

assert pos == end and fixup_off is not None and data_off is not None
assert fixup_off + 28 <= len(image)
version, starts, imports, names, count, fmt, symbol_fmt = struct.unpack_from(
    "<7I", image, fixup_off
)
assert version == 0 and symbol_fmt == 0
assert fmt in (1, 2, 3), "unsupported chained import table format"


def import_at(index):
    assert index < count
    stride = {1: 4, 2: 8, 3: 16}[fmt]
    off = fixup_off + imports + index * stride
    assert off + stride <= len(image)
    if fmt == 3:
        raw = u64(off)
        dylib = sign(raw & 0xFFFF, 16)
        weak = bool((raw >> 16) & 1)
        name_offset = raw >> 32
        addend = struct.unpack_from("<q", image, off + 8)[0]
    else:
        raw = u32(off)
        dylib = sign(raw & 0xFF, 8)
        weak = bool((raw >> 8) & 1)
        name_offset = raw >> 9
        addend = struct.unpack_from("<i", image, off + 4)[0] if fmt == 2 else 0
    start = fixup_off + names + name_offset
    assert 0 <= start < len(image)
    stop = image.index(0, start)
    name = image[start:stop].decode("ascii")
    return name, dylib, weak, addend


# Import ordinals are dyld's resolution policy, not just a spelling check:
# ordinary dylib is 1 (sole explicit dylib), weak lookup is -3, and unresolved
# plugin imports use flat lookup -2 to resolve against the host executable.
expected = (
    ("_external", 1, 0, 12, True),
    ("_weak", -3, 1, 34, False),
    ("_host", -2, 2, 56, True),
    ("_local", None, 3, 78, False),
)
for i, (symbol, ordinal, key, diversity, addr_div) in enumerate(expected):
    raw = u64(data_off + i * 8)
    authenticated = (raw >> 63) & 1
    bound = (raw >> 62) & 1
    assert authenticated == 1, f"{symbol}: missing authenticated pointer"
    assert bound == int(ordinal is not None), f"{symbol}: wrong bind/rebase"
    assert ((raw >> 49) & 3) == key, f"{symbol}: wrong PAC key"
    assert ((raw >> 32) & 0xFFFF) == diversity, f"{symbol}: wrong diversity"
    assert bool((raw >> 48) & 1) == addr_div, f"{symbol}: address diversity"
    if ordinal is None:
        continue
    actual_name, actual_ordinal, weak_import, addend = import_at(raw & 0xFFFFFF)
    assert actual_name == symbol, f"{symbol}: bound to {actual_name}"
    assert actual_ordinal == ordinal, (
        f"{symbol}: expected dylib ordinal {ordinal}, got {actual_ordinal}"
    )
    assert addend == 0, f"{symbol}: unexpected import addend"
    assert not weak_import, f"{symbol}: unexpected weak reference flag"

print("ARM64e symbol resolution: dylib=1 weak=-3 dynamic=-2; PAC metadata preserved")
