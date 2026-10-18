"""Structural regression tests for the experimental COFF-to-LE VxD writer."""
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest

WRITER = Path(__file__).resolve().parents[1] / "whp-le-link.py"
spec = importlib.util.spec_from_file_location("whp_le_link", WRITER)
module = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = module
spec.loader.exec_module(module)


def fixture():
    # Minimal i386 COFF with a public DDB symbol, no relocations.
    data = bytearray(20 + 40 + 16 + 18 + 4)
    struct.pack_into("<HHIIIHH", data, 0, 0x14c, 1, 0, 76, 1, 0, 0)
    data[20:28] = b".data\0\0\0"
    struct.pack_into("<IIIIIIHHI", data, 28, 0, 0, 16, 60, 0, 0,
                     0, 0, 0xC0300040)
    data[60:76] = b"\0" * 16
    data[76:84] = b"_WHPTEST"
    struct.pack_into("<IhHBB", data, 84, 0, 1, 0, 2, 0)
    struct.pack_into("<I", data, 94, 4)
    return bytes(data)


class WriterTests(unittest.TestCase):
    def test_vxd_header_and_ordinal(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "probe.obj"
            path.write_bytes(fixture())
            img = module.link([module.parse_coff(path)], "WHPTEST", "_WHPTEST")
            self.assertEqual(img[:2], b"MZ")
            self.assertEqual(img[0x80:0x82], b"LE")
            self.assertEqual(struct.unpack_from("<H", img, 0x8A)[0], 4)
            self.assertEqual(struct.unpack_from("<I", img, 0x90)[0], 0x38000)
            entry = struct.unpack_from("<I", img, 0x80 + 0x5C)[0] + 0x80
            self.assertEqual(img[entry:entry + 2], b"\x01\x03")
            self.assertEqual(struct.unpack_from("<H", img, entry + 2)[0], 1)

    def test_reject_pe_or_bad_cpu(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.obj"
            path.write_bytes(b"MZ" + b"\0" * 128)
            with self.assertRaises(module.LinkError):
                module.parse_coff(path)

    def test_reject_undefined_export(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "probe.obj"
            path.write_bytes(fixture())
            with self.assertRaises(module.LinkError):
                module.link([module.parse_coff(path)], "WHPTEST", "_MISSING")

    def test_invalid_name(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "probe.obj"
            path.write_bytes(fixture())
            with self.assertRaises(module.LinkError):
                module.link([module.parse_coff(path)], "lowercase", "_WHPTEST")


if __name__ == "__main__":
    unittest.main()
