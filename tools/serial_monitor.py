#!/usr/bin/env python3
"""
Patch 060B ESP32 serial monitor helper.

This is a small alternative to `idf.py monitor` for the common case where you
only want to watch UART output and exit with normal Ctrl+C instead of Ctrl+].

It intentionally does not replace ESP-IDF's advanced monitor features such as
ELF backtrace decoding. For decoded crashes, keep using `idf.py monitor`.
"""

from __future__ import annotations

import argparse
import glob
import os
import sys
import time
from datetime import datetime
from typing import Iterable, List, Optional

DEFAULT_BAUD = 115200
DEFAULT_PATTERNS = (
    "/dev/cu.usbmodem*",
    "/dev/cu.usbserial*",
    "/dev/cu.SLAB_USBtoUART*",
    "/dev/cu.wchusbserial*",
    "/dev/ttyACM*",
    "/dev/ttyUSB*",
)


def discover_ports(patterns: Iterable[str] = DEFAULT_PATTERNS) -> List[str]:
    ports: List[str] = []
    seen = set()
    for pattern in patterns:
        for path in sorted(glob.glob(pattern)):
            if path not in seen and os.path.exists(path):
                seen.add(path)
                ports.append(path)
    return ports


def choose_port(requested: Optional[str]) -> str:
    if requested:
        return requested
    ports = discover_ports()
    if not ports:
        raise SystemExit(
            "ERROR: no serial port found. Connect the ESP32-S3 or pass "
            "--port /dev/cu.usbmodem101."
        )
    if len(ports) > 1:
        lines = ["ERROR: multiple serial ports found; choose one with --port:"]
        lines.extend(f"  {p}" for p in ports)
        raise SystemExit("\n".join(lines))
    return ports[0]


def load_pyserial():
    try:
        import serial  # type: ignore
    except ModuleNotFoundError as exc:
        raise SystemExit(
            "ERROR: pyserial is not installed. Install it with:\n"
            "  python3 -m pip install pyserial\n"
            "or, inside a virtual environment:\n"
            "  python3 -m pip install pyserial"
        ) from exc
    return serial


def ts_prefix() -> str:
    return datetime.now().strftime("[%H:%M:%S] ")


def monitor(args: argparse.Namespace) -> int:
    if args.list:
        ports = discover_ports()
        if ports:
            print("Detected serial ports:")
            for port in ports:
                print(f"  {port}")
        else:
            print("No serial ports detected.")
        return 0

    serial = load_pyserial()
    port = choose_port(args.port)

    try:
        ser = serial.Serial(
            port=port,
            baudrate=args.baud,
            timeout=args.timeout,
            write_timeout=1.0,
        )
    except Exception as exc:  # pyserial raises platform-specific subclasses
        raise SystemExit(f"ERROR: could not open {port}: {exc}") from exc

    with ser:
        if args.reset:
            # Common ESP32 reset pulse. Kept opt-in to avoid surprising resets.
            ser.dtr = False
            ser.rts = True
            time.sleep(0.1)
            ser.rts = False
            time.sleep(0.1)

        print(f"ESP32 serial monitor on {port} @ {args.baud} baud")
        print("Press Ctrl+C to stop. Use idf.py monitor if you need backtrace decoding.")
        sys.stdout.flush()

        at_line_start = True
        try:
            while True:
                data = ser.read(args.chunk_size)
                if not data:
                    continue
                if args.raw:
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
                    continue

                text = data.decode(args.encoding, errors="replace")
                if args.timestamps:
                    # Prefix each new printed line without buffering full lines.
                    out_parts = []
                    for ch in text:
                        if at_line_start:
                            out_parts.append(ts_prefix())
                            at_line_start = False
                        out_parts.append(ch)
                        if ch == "\n":
                            at_line_start = True
                    sys.stdout.write("".join(out_parts))
                else:
                    sys.stdout.write(text)
                sys.stdout.flush()
        except KeyboardInterrupt:
            print("\nSerial monitor stopped by Ctrl+C.")
            return 0


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Simple ESP32 serial monitor that exits with Ctrl+C. "
            "Use this after `idf.py flash` when you do not need ESP-IDF monitor shortcuts."
        )
    )
    parser.add_argument("--port", help="Serial port, e.g. /dev/cu.usbmodem101")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Baud rate, default {DEFAULT_BAUD}")
    parser.add_argument("--list", action="store_true", help="List detected serial ports and exit")
    parser.add_argument("--reset", action="store_true", help="Pulse RTS/DTR to reset ESP32 after opening the port")
    parser.add_argument("--raw", action="store_true", help="Write raw bytes to stdout without decoding")
    parser.add_argument("--timestamps", action="store_true", help="Prefix printed serial lines with local time")
    parser.add_argument("--encoding", default="utf-8", help="Text encoding for serial bytes, default utf-8")
    parser.add_argument("--timeout", type=float, default=0.2, help="Read timeout in seconds, default 0.2")
    parser.add_argument("--chunk-size", type=int, default=256, help="Serial read chunk size, default 256 bytes")
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    return monitor(args)


if __name__ == "__main__":
    raise SystemExit(main())
