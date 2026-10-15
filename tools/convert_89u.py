#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Convert a TI-89 Titanium .89u upgrade into a private raw flash image.

The layout intentionally follows TiEmu 3.04's `ti68k_convert_tib_to_image`
behavior, but emits only the 4 MiB flash bytes. TiEmu's host-dependent `.img`
header is not used by Tiny89.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


TIFL_SIGNATURE = b"**TIFL**"
TIFL_HEADER_SIZE = 78
TI89_DEVICE_TYPE = 0x98
TI89_OS_DATA_TYPE = 0x23
TI89T_HARDWARE_ID = 9

SYSTEM_PART_OFFSET = 0x12000
BOOT_OFFSET = 0x88
BOOT_SIZE = 256
FLASH_SIZE = 4 * 1024 * 1024
PARAMETER_POINTER_OFFSET = 0x104
PARAMETER_BLOCK_OFFSET = 0x108
PARAMETER_BLOCK_LENGTH = 24


class ConversionError(Exception):
    """Raised when an input cannot be converted safely."""


@dataclass(frozen=True)
class TiflRecord:
    revision_major: int
    revision_minor: int
    day: int
    month: int
    year: int
    name: str
    device_type: int
    data_type: int
    hardware_id: int
    payload: bytes


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_records(source: bytes) -> list[TiflRecord]:
    records: list[TiflRecord] = []
    offset = 0

    while offset < len(source):
        if len(source) - offset < TIFL_HEADER_SIZE:
            raise ConversionError(f"truncated TIFL header at offset 0x{offset:x}")

        header = source[offset : offset + TIFL_HEADER_SIZE]
        if header[:8] != TIFL_SIGNATURE:
            raise ConversionError(f"invalid TIFL signature at offset 0x{offset:x}")

        payload_length = struct.unpack_from("<I", header, 74)[0]
        payload_start = offset + TIFL_HEADER_SIZE
        payload_end = payload_start + payload_length
        if payload_end > len(source):
            raise ConversionError(
                f"record at offset 0x{offset:x} claims {payload_length} bytes, "
                f"but only {len(source) - payload_start} remain"
            )

        records.append(
            TiflRecord(
                revision_major=header[8],
                revision_minor=header[9],
                day=header[12],
                month=header[13],
                year=struct.unpack_from(">H", header, 14)[0],
                name=header[17:25].split(b"\0", 1)[0].decode("ascii", "replace"),
                device_type=header[48],
                data_type=header[49],
                hardware_id=header[73],
                payload=source[payload_start:payload_end],
            )
        )
        offset = payload_end

    if not records:
        raise ConversionError("input contains no TIFL records")
    return records


def be16(value: int) -> bytes:
    return struct.pack(">H", value)


def be32(value: int) -> bytes:
    return struct.pack(">I", value)


def bcd_byte(value: int) -> int:
    high, low = value >> 4, value & 0x0F
    if high > 9 or low > 9:
        raise ConversionError(f"invalid packed-BCD byte 0x{value:02x}")
    return high * 10 + low


def find_version(payload: bytes) -> str:
    # TiEmu scans the relocated system part on 16-bit boundaries. The common
    # AMS form is N.NN followed by NUL and a build date.
    for offset in range(0, max(0, len(payload) - 16), 2):
        candidate = payload[offset : offset + 16]
        if (
            len(candidate) >= 5
            and 48 <= candidate[0] <= 57
            and candidate[1] == ord(".")
            and 48 <= candidate[2] <= 57
            and 48 <= candidate[3] <= 57
            and candidate[4] == 0
        ):
            return candidate[:4].decode("ascii")
    return "?.??"


def build_flash(record: TiflRecord, hardware_type: int) -> tuple[bytes, dict[str, object]]:
    payload = record.payload
    minimum = BOOT_OFFSET + BOOT_SIZE
    if len(payload) < minimum:
        raise ConversionError(
            f"OS payload is {len(payload)} bytes; at least {minimum} are required"
        )
    if SYSTEM_PART_OFFSET + len(payload) > FLASH_SIZE:
        raise ConversionError("OS payload does not fit in a 4 MiB flash image")
    if record.device_type != TI89_DEVICE_TYPE:
        raise ConversionError(
            f"expected TI-89 device type 0x{TI89_DEVICE_TYPE:02x}, "
            f"found 0x{record.device_type:02x}"
        )
    if record.data_type != TI89_OS_DATA_TYPE:
        raise ConversionError(
            f"expected OS data type 0x{TI89_OS_DATA_TYPE:02x}, "
            f"found 0x{record.data_type:02x}"
        )

    rom_base = payload[BOOT_OFFSET + 5] & 0xF0
    if rom_base != 0x80:
        raise ConversionError(
            f"upgrade boot vector selects ROM base 0x{rom_base:02x}; "
            "this converter accepts TI-89 Titanium images only"
        )

    flash = bytearray(b"\xff" * FLASH_SIZE)
    flash[:BOOT_SIZE] = payload[BOOT_OFFSET : BOOT_OFFSET + BOOT_SIZE]

    # TiEmu places a marker, a 32-bit pointer to 0x800108, then the 24-byte
    # big-endian hardware parameter block used for its synthetic image.
    flash[0x100:0x104] = b"\xfe\xed\xba\xbe"
    flash[PARAMETER_POINTER_OFFSET : PARAMETER_POINTER_OFFSET + 4] = be32(
        (rom_base << 16) | PARAMETER_BLOCK_OFFSET
    )
    parameters = (
        be16(PARAMETER_BLOCK_LENGTH)
        + be32(TI89T_HARDWARE_ID)
        + be32(2)  # hardware revision
        + be32(1)  # boot major
        + be32(2)  # TiEmu emits hardware revision in boot-revision position
        + be32(1)  # boot build
        + be32(hardware_type)
    )
    flash[PARAMETER_BLOCK_OFFSET : PARAMETER_BLOCK_OFFSET + len(parameters)] = parameters
    flash[SYSTEM_PART_OFFSET : SYSTEM_PART_OFFSET + len(payload)] = payload

    metadata: dict[str, object] = {
        "format": "tiny89-raw-flash-v1",
        "calculator": "TI-89 Titanium",
        "flash_size": FLASH_SIZE,
        "rom_base": f"0x{rom_base:02x}0000",
        "hardware_type": hardware_type,
        "hardware_id": record.hardware_id,
        "os_version": find_version(payload),
        "tifl_revision": f"{record.revision_major}.{record.revision_minor}",
        "tifl_date": (
            f"{bcd_byte(record.year >> 8):02d}{bcd_byte(record.year & 0xff):02d}-"
            f"{bcd_byte(record.month):02d}-{bcd_byte(record.day):02d}"
        ),
        "tifl_name": record.name,
        "payload_size": len(payload),
        "payload_sha256": sha256(payload),
    }
    return bytes(flash), metadata


def convert(input_path: Path, output_path: Path, hardware_type: int) -> dict[str, object]:
    source = input_path.read_bytes()
    records = parse_records(source)
    flash, metadata = build_flash(records[-1], hardware_type)
    metadata = {
        "source_size": len(source),
        "source_sha256": sha256(source),
        "record_count": len(records),
        **metadata,
        "flash_sha256": sha256(flash),
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = output_path.with_name(f".{output_path.name}.tmp-{os.getpid()}")
    try:
        temporary.write_bytes(flash)
        temporary.replace(output_path)
    finally:
        temporary.unlink(missing_ok=True)

    manifest_path = output_path.with_suffix(output_path.suffix + ".json")
    manifest_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")
    return metadata


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="private TI-89 Titanium .89u file")
    parser.add_argument("output", type=Path, help="private 4 MiB raw flash output")
    parser.add_argument(
        "--hardware-type",
        type=int,
        choices=(3, 4),
        default=3,
        help="TiEmu hardware generation to synthesize (default: 3)",
    )
    args = parser.parse_args()

    try:
        metadata = convert(args.input, args.output, args.hardware_type)
    except (ConversionError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    print(f"wrote {args.output} ({metadata['flash_size']} bytes)")
    print(f"OS version: {metadata['os_version']}")
    print(f"source SHA-256: {metadata['source_sha256']}")
    print(f"flash SHA-256:  {metadata['flash_sha256']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
