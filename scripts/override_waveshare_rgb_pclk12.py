#!/usr/bin/env python3
"""Generate a build-local copy of the locked Waveshare BSP with PCLK=12 MHz."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import sys


EXPECTED_SOURCE_SHA256 = "0f3bbe997b4055620e93edc64b9628c5c5f8160ff6275367d8570dc40fa1ec4e"
ANCHOR = "    rgb_config.timings.h_res = BSP_LCD_H_RES;\n"
OVERRIDE = (
    "    /* Patch102 build-local experiment: retain all BSP timing except PCLK. */\n"
    "    rgb_config.timings.pclk_hz = 12 * 1000 * 1000;\n"
)


def generate(source: Path, output: Path) -> str:
    raw = source.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED_SOURCE_SHA256:
        raise ValueError(f"locked BSP source SHA-256 mismatch: {digest}")
    text = raw.decode("utf-8")
    if text.count(ANCHOR) != 1:
        raise ValueError("expected exactly one BSP RGB timing insertion anchor")
    generated = text.replace(ANCHOR, OVERRIDE + ANCHOR, 1)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8", newline="")
    return digest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        digest = generate(args.source, args.output)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"Patch102 source override rejected: {exc}", file=sys.stderr)
        return 2
    print(f"source_sha256={digest} requested_pclk_hz=12000000 changed_fields=pclk_hz")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
