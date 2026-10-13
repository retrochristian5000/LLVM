#!/usr/bin/env python3
"""Verify ARM64e LC_ID_DYLIB/LC_LOAD_DYLIB metadata, not printed text."""
import struct
import sys
from pathlib import Path

if len(sys.argv) not in (4, 7):
    raise SystemExit(
        "usage: check.py image filetype (id|load|none) [install-name current compat]"
    )

data = Path(sys.argv[1]).read_bytes()
expect_type = int(sys.argv[2])
kind = sys.argv[3]
assert kind in ("id", "load", "none")
assert len(sys.argv) == (4 if kind == "none" else 7)
if len(data) < 32:
    raise AssertionError("truncated Mach-O header")
magic, cpu, subtype, filetype, ncmds, sizeofcmds, flags, reserved = (
    struct.unpack_from("<8I", data, 0)
)
assert (magic, cpu, subtype, filetype) == (
    0xFEEDFACF, 0x0100000C, 0x80000002, expect_type
), "wrong Mach-O ARM64e ABI or filetype"
assert 32 + sizeofcmds <= len(data)

records = []
position = 32
for _ in range(ncmds):
    if position + 8 > 32 + sizeofcmds:
        raise AssertionError("load command outside Mach-O header")
    cmd, size = struct.unpack_from("<II", data, position)
    if size < 8 or position + size > 32 + sizeofcmds:
        raise AssertionError("invalid load command")
    if cmd in (0xD, 0xC):  # LC_ID_DYLIB, LC_LOAD_DYLIB
        if size < 24:
            raise AssertionError("truncated dylib load command")
        name_offset, timestamp, current, compat = struct.unpack_from(
            "<4I", data, position + 8
        )
        if not 24 <= name_offset < size:
            raise AssertionError("invalid dylib install name offset")
        name_start = position + name_offset
        terminator = data.find(b"\0", name_start, position + size)
        if terminator < 0:
            raise AssertionError("unterminated dylib install name")
        name = data[name_start:terminator].decode("utf-8")
        records.append((cmd, name, current, compat))
    position += size
assert position == 32 + sizeofcmds

if kind == "none":
    assert not any(cmd == 0xD for cmd, _, _, _ in records), (
        "MH_BUNDLE unexpectedly contains LC_ID_DYLIB"
    )
else:
    name = sys.argv[4]
    def encode(version):
        fields = [int(n) for n in version.split(".")]
        if not 1 <= len(fields) <= 3:
            raise AssertionError("invalid expected version")
        fields += [0] * (3 - len(fields))
        assert fields[0] <= 65535 and fields[1] <= 255 and fields[2] <= 255
        return fields[0] << 16 | fields[1] << 8 | fields[2]
    requested = (0xD if kind == "id" else 0xC, name,
                 encode(sys.argv[5]), encode(sys.argv[6]))
    assert records.count(requested) == 1, (
        f"{kind}: expected {requested}, got {records}"
    )
    if kind == "id":
        assert sum(cmd == 0xD for cmd, _, _, _ in records) == 1, (
            "a dylib must have exactly one LC_ID_DYLIB"
        )
print(f"ARM64e Mach-O {kind} identity/version metadata verified: {sys.argv[1]}")
