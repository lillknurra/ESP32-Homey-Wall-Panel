#!/usr/bin/env python3
"""Measure the ESP32-S3 status handler frame from the generated IDF build."""

from __future__ import annotations

import json
import os
import re
import shlex
import subprocess
import tempfile
from pathlib import Path


FRAME_LIMIT_BYTES = 512
HTTPD_DEFAULT_STACK_BYTES = 4096
SOURCE_SUFFIX = "/components/secure_bootstrap/athom_oauth_runtime.c"


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def main() -> None:
    root = Path(__file__).resolve().parents[3]
    build = root / "build"
    compile_commands = build / "compile_commands.json"
    if not compile_commands.is_file():
        fail("build/compile_commands.json is missing; run the ESP-IDF build first")

    entries = json.loads(compile_commands.read_text(encoding="utf-8"))
    entry = next(
        (item for item in entries if item.get("file", "").endswith(SOURCE_SUFFIX)),
        None,
    )
    if entry is None:
        fail("compile database has no athom_oauth_runtime.c entry")

    command = entry.get("arguments") or shlex.split(entry["command"])
    command = list(command)
    try:
        output_index = command.index("-o")
        source_index = len(command) - 1
        if command[source_index].endswith(SOURCE_SUFFIX) is False:
            source_index = next(
                i for i, arg in enumerate(command) if arg.endswith(SOURCE_SUFFIX)
            )
    except (ValueError, StopIteration):
        fail("could not locate compiler output/source arguments")

    source = root / "components/secure_bootstrap/athom_oauth_runtime.c"
    if not source.is_file():
        fail("status handler source file is missing")

    with tempfile.TemporaryDirectory(prefix="athom_http_stack_usage_") as temp:
        output = Path(temp) / "athom_oauth_runtime.o"
        command[output_index + 1] = str(output)
        command[source_index] = str(source)
        command.insert(output_index, "-fstack-usage")
        result = subprocess.run(
            command,
            cwd=entry["directory"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode != 0:
            fail("Xtensa compiler failed:\n" + result.stderr[-4000:])

        usage_file = output.with_suffix(".su")
        if not usage_file.is_file():
            fail("compiler did not produce stack-usage information")
        match = re.search(
            r"(?:^|/)athom_oauth_runtime\.c:\d+:\d+:status_get\t(\d+)\t(\w+)",
            usage_file.read_text(encoding="utf-8"),
            re.MULTILINE,
        )
        if match is None:
            fail("stack-usage output does not contain status_get")
        frame_bytes = int(match.group(1))
        frame_kind = match.group(2)
        if frame_kind != "static":
            fail(f"status_get stack frame is not statically bounded: {frame_kind}")
        if frame_bytes > FRAME_LIMIT_BYTES:
            fail(
                f"status_get frame {frame_bytes} exceeds {FRAME_LIMIT_BYTES}-byte guard"
            )

    idf_path = Path(os.environ.get("IDF_PATH", "~/GitHub/esp-idf-v6.0.1")).expanduser()
    httpd_header = idf_path / "components/esp_http_server/include/esp_http_server.h"
    if not httpd_header.is_file():
        fail("ESP-IDF HTTP server header is unavailable at IDF_PATH")
    header_text = httpd_header.read_text(encoding="utf-8")
    default_config = re.search(
        r"#define\s+HTTPD_DEFAULT_CONFIG\(\).*?\.stack_size\s*=\s*(\d+)",
        header_text,
        re.DOTALL,
    )
    if default_config is None or int(default_config.group(1)) != HTTPD_DEFAULT_STACK_BYTES:
        fail("HTTPD_DEFAULT_CONFIG stack is not the audited 4096 bytes")

    print(
        "PASS: ESP32-S3 status_get frame is "
        f"{frame_bytes} bytes (static); guard={FRAME_LIMIT_BYTES}; "
        f"HTTPD default={HTTPD_DEFAULT_STACK_BYTES} bytes"
    )


if __name__ == "__main__":
    main()
