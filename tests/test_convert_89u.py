# SPDX-License-Identifier: GPL-2.0-or-later
from __future__ import annotations

import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("convert_89u", ROOT / "tools/convert_89u.py")
assert SPEC and SPEC.loader
CONVERTER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = CONVERTER
SPEC.loader.exec_module(CONVERTER)


def upgrade(rom_base: int = 0x80) -> bytes:
    payload = bytearray(b"\xff" * 0x300)
    payload[CONVERTER.BOOT_OFFSET + 5] = rom_base
    payload[0x200:0x210] = b"3.10\x0007/18/2005"
    header = bytearray(78)
    header[:8] = b"**TIFL**"
    header[8:16] = bytes.fromhex("0310000018072005")
    header[17:25] = b"basecode"
    header[48] = CONVERTER.TI89_DEVICE_TYPE
    header[49] = CONVERTER.TI89_OS_DATA_TYPE
    header[73] = CONVERTER.TI89T_HARDWARE_ID
    struct.pack_into("<I", header, 74, len(payload))
    return bytes(header + payload)


class ConverterTest(unittest.TestCase):
    def test_titanium_upgrade(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "os.89u"
            output = Path(directory) / "os.raw"
            source.write_bytes(upgrade())
            metadata = CONVERTER.convert(source, output, 3)
            raw = output.read_bytes()

        self.assertEqual(len(raw), CONVERTER.FLASH_SIZE)
        self.assertEqual(raw[:8], upgrade()[78 + 0x88 : 78 + 0x90])
        self.assertEqual(raw[0x100:0x108], bytes.fromhex("feedbabe00800108"))
        self.assertEqual(raw[0x108:0x10e], bytes.fromhex("001800000009"))
        self.assertEqual(metadata["calculator"], "TI-89 Titanium")
        self.assertEqual(metadata["os_version"], "3.10")
        self.assertEqual(metadata["tifl_date"], "2005-07-18")

    def test_rejects_non_titanium_base(self) -> None:
        record = CONVERTER.parse_records(upgrade(0x20))[0]
        with self.assertRaises(CONVERTER.ConversionError):
            CONVERTER.build_flash(record, 3)

    def test_rejects_truncated_record(self) -> None:
        with self.assertRaises(CONVERTER.ConversionError):
            CONVERTER.parse_records(upgrade()[:-1])


if __name__ == "__main__":
    unittest.main()
