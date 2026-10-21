#!/usr/bin/env python3
"""Compile the production awning setter and roller model with synthetic state."""
from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
source = (component / "athom_cloud_client.c").read_text(encoding="utf-8")
signature = "athom_homey_awning_write_result_t athom_cloud_set_awning_position("
start = source.find(signature)
if start < 0:
    raise SystemExit("production awning position setter missing")
brace = source.find("{", start)
depth = 0
end = None
for index in range(brace, len(source)):
    if source[index] == "{":
        depth += 1
    elif source[index] == "}":
        depth -= 1
        if depth == 0:
            end = index + 1
            break
if end is None:
    raise SystemExit("production awning position setter body incomplete")
production = source[start:end]

harness = Path(__file__).with_name("test_patch104_awning_middle_target.c").read_text(
    encoding="utf-8")
marker = "/* PATCH104_PRODUCTION_FUNCTION */"
if harness.count(marker) != 1:
    raise SystemExit("production insertion marker must occur exactly once")

with tempfile.TemporaryDirectory(prefix="patch104_middle_target_") as temporary:
    cfile = Path(temporary) / "middle_target_test.c"
    binary = Path(temporary) / "middle_target_test"
    cfile.write_text(harness.replace(marker, production), encoding="utf-8")
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(component / "include"), str(cfile),
        str(component / "panel_roller_model.c"), "-lm", "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
