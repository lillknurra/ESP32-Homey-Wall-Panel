#!/usr/bin/env python3
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
repo = component.parents[1]
idf_path = Path(os.environ.get(
    "IDF_PATH",
    str(Path.home() / "GitHub" / "esp-idf-v6.0.1"),
)).expanduser().resolve()

candidates = []

override = os.environ.get("PANEL_HOMEY_CJSON_DIR")
if override:
    candidates.append(Path(override).expanduser().resolve())

# The project uses the Component Manager package `espressif__cjson`.
candidates.append(repo / "managed_components" / "espressif__cjson" / "cJSON")

# Compatibility fallback for ESP-IDF layouts that vendor cJSON directly.
candidates.append(idf_path / "components" / "json" / "cJSON")

cjson_dir = next(
    (
        candidate
        for candidate in candidates
        if (candidate / "cJSON.c").is_file()
        and (candidate / "cJSON.h").is_file()
    ),
    None,
)

if cjson_dir is None:
    rendered = "\n".join(f"  - {candidate}" for candidate in candidates)
    raise SystemExit(
        "cJSON source not found. Checked:\n"
        f"{rendered}"
    )

cjson_source = cjson_dir / "cJSON.c"

with tempfile.TemporaryDirectory() as temporary_directory:
    binary = Path(temporary_directory) / "test_panel_homey_read_snapshot"
    command = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-I",
        str(component / "include"),
        "-I",
        str(cjson_dir),
        str(component / "panel_homey_read_snapshot.c"),
        str(cjson_source),
        str(Path(__file__).with_name("test_panel_homey_read_snapshot.c")),
        *shlex.split(os.environ.get("PANEL_HOMEY_CJSON_LIBS", "")),
        "-lm",
        "-o",
        str(binary),
    ]
    print("CJSON_DIR:", cjson_dir)
    print("HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True, cwd=repo)
    subprocess.run([str(binary)], check=True, cwd=repo)
    print("PANEL_HOMEY_READ_SNAPSHOT_HOST_RUNNER PASS")
