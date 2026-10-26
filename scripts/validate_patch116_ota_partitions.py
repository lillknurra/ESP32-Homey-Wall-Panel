#!/usr/bin/env python3
"""Fail-closed, host-only static checks for the Patch116 *candidate* partition CSV.

This is not a replacement for ESP-IDF v6.0.1 gen_esp32part.py, nor evidence that
bootloader/partition-table migration is electrically safe. No device I/O.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import sys

FLASH_BYTES = 16 * 1024 * 1024
PARTITION_TABLE_OFFSET = 0x8000
SECTOR_BYTES = 0x1000
APP_ALIGNMENT = 0x10000
APP_SLOT_MIN_BYTES = 0x400000
MIN_IMAGE_BYTES = 0x1000

EXPECTED_OLD = {
    "nvs": ("data", "nvs", 0x9000, 0x6000),
    "phy_init": ("data", "phy", 0xF000, 0x1000),
    "factory": ("app", "factory", 0x10000, 0x600000),
    "diaglog": ("data", "undefined", 0x610000, 0x100000),
}
EXPECTED_NEW = {
    "otadata": ("data", "ota", 0x710000, 0x2000),
    "ota_0": ("app", "ota_0", 0x720000, 0x400000),
    "ota_1": ("app", "ota_1", 0xB20000, 0x400000),
}

class PartitionError(ValueError):
    pass

@dataclass(frozen=True)
class Partition:
    name: str
    kind: str
    subtype: str
    offset: int
    size: int
    flags: str

    @property
    def end(self) -> int:
        return self.offset + self.size


def _number(raw: str, field: str) -> int:
    try:
        # int(x, 0) accepts decimal/0x; IDF's CSV also allows N/M/K, not needed here.
        value = int(raw, 0)
    except ValueError as exc:
        raise PartitionError(f"invalid {field} number {raw!r}") from exc
    if value < 0:
        raise PartitionError(f"negative {field}: {raw}")
    return value


def parse(text: str) -> dict[str, Partition]:
    parts: dict[str, Partition] = {}
    for line_number, line in enumerate(text.splitlines(), 1):
        clean = line.split("#", 1)[0].strip()
        if not clean:
            continue
        cols = [col.strip() for col in clean.split(",")]
        if len(cols) not in (5, 6):
            raise PartitionError(f"line {line_number}: expected five or six columns")
        if len(cols) == 5:
            cols.append("")
        name, kind, subtype, offset, size, flags = cols
        if not name or not kind or not subtype or not offset or not size:
            raise PartitionError(f"line {line_number}: empty mandatory field")
        if name in parts:
            raise PartitionError(f"duplicate partition name {name}")
        if len(name.encode("ascii", "strict")) >= 16:
            raise PartitionError(f"partition name too long: {name}")
        if flags:
            raise PartitionError(f"unexpected flash partition flags for {name}")
        if kind not in ("app", "data"):
            raise PartitionError(f"unexpected type for {name}: {kind}")
        part = Partition(name, kind, subtype, _number(offset, "offset"),
                         _number(size, "size"), flags)
        if part.size == 0 or part.offset % SECTOR_BYTES or part.size % SECTOR_BYTES:
            raise PartitionError(f"partition {name}: not 4KiB aligned or has zero size")
        if part.kind == "app" and part.offset % APP_ALIGNMENT:
            raise PartitionError(f"app partition {name}: offset must be 64KiB aligned")
        if part.end > FLASH_BYTES:
            raise PartitionError(f"partition {name}: exceeds 16MiB flash")
        parts[name] = part
    if not parts:
        raise PartitionError("no partitions")
    return parts


def _assert_expected(parts: dict[str, Partition], expected: dict[str, tuple], name: str) -> None:
    if set(parts) != set(expected):
        raise PartitionError(f"{name}: names differ, missing={sorted(set(expected)-set(parts))}, "
                             f"unexpected={sorted(set(parts)-set(expected))}")
    for key, values in expected.items():
        p = parts[key]
        if (p.kind, p.subtype, p.offset, p.size) != values:
            raise PartitionError(f"{name}: {key} is not at its expected type/offset/size")


def _check_overlaps(parts: dict[str, Partition]) -> None:
    # Protect bootloader + physical partition table region as well as all CSV entries.
    sorted_parts = sorted(parts.values(), key=lambda p: (p.offset, p.end))
    previous_end = PARTITION_TABLE_OFFSET + SECTOR_BYTES
    for p in sorted_parts:
        if p.offset < previous_end:
            raise PartitionError(f"partition overlap or reserved boot area at {p.name}")
        previous_end = p.end


def validate(original_csv: str, proposed_csv: str, image_sizes: tuple[int, ...] = ()) -> dict:
    old = parse(original_csv)
    new = parse(proposed_csv)
    _assert_expected(old, EXPECTED_OLD, "current baseline")
    _assert_expected(new, {**EXPECTED_OLD, **EXPECTED_NEW}, "OTA candidate")
    _check_overlaps(old)
    _check_overlaps(new)
    for name in EXPECTED_OLD:
        if old[name] != new[name]:
            raise PartitionError(f"existing partition changed: {name}")
    if new["otadata"].size != 0x2000:
        raise PartitionError("otadata must be 0x2000")
    slots = [new["ota_0"], new["ota_1"]]
    if any(slot.size < APP_SLOT_MIN_BYTES for slot in slots):
        raise PartitionError("OTA slot smaller than minimum")
    slot_size = min(p.size for p in slots)
    for image_size in image_sizes:
        if image_size < MIN_IMAGE_BYTES or image_size > slot_size:
            raise PartitionError(f"image {image_size} bytes does not fit OTA slots")
    return {
        "result": "PASS_STATIC_PARTITION_LAYOUT_ONLY",
        "flash_bytes": FLASH_BYTES,
        "existing_partitions_preserved": list(EXPECTED_OLD),
        "new_partitions": [
            {"name": p.name, "offset": f"0x{p.offset:X}",
             "end_exclusive": f"0x{p.end:X}", "size_bytes": p.size}
            for p in (new["otadata"], *slots)
        ],
        "ota_slot_bytes": slot_size,
        "tail_free_bytes": FLASH_BYTES - max(p.end for p in new.values()),
        "checked_image_sizes_bytes": image_sizes,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    root = Path(__file__).resolve().parent.parent
    parser.add_argument("--baseline", type=Path, default=root / "partitions.csv")
    parser.add_argument("--candidate", type=Path, default=root / "partitions_ota_candidate.csv")
    parser.add_argument("--image-bytes", type=int, action="append", default=[])
    args = parser.parse_args()
    try:
        result = validate(args.baseline.read_text(encoding="utf-8"),
                          args.candidate.read_text(encoding="utf-8"),
                          tuple(args.image_bytes))
    except (OSError, PartitionError, UnicodeError) as exc:
        print(f"PATCH116_PARTITION_CHECK=FAIL reason={exc}", file=sys.stderr)
        return 1
    print("PATCH116_PARTITION_CHECK=" + result["result"])
    for name in result["existing_partitions_preserved"]:
        print(f"PRESERVED={name}")
    for p in result["new_partitions"]:
        print(f"NEW={p['name']} offset={p['offset']} size={p['size_bytes']}")
    print(f"OTA_SLOT_BYTES={result['ota_slot_bytes']}")
    print(f"TAIL_FREE_BYTES={result['tail_free_bytes']}")
    print("IMPORTANT=ESP_IDF_GENERATOR_BUILD_BOOTLOADER_SECURITY_AND_MIGRATION_NOT_VERIFIED")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
