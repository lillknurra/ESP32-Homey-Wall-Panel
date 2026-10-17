#!/usr/bin/env python3
"""Compile the real binding/model/roller pipeline; optional historical binding source."""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--binding-source", type=Path)
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
component = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="patch086_readback_") as temporary:
    binary = Path(temporary) / "readback"
    command = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
               "-I", str(component / "include")]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    command += [str(args.binding_source or component / "panel_homey_dashboard_binding.c")]
    command += [str(component / f) for f in ("panel_ui_model.c", "panel_roller_model.c", "panel_ui_store.c")]
    command += [str(Path(__file__).with_name("test_patch086_display_readback.c")), "-lm", "-o", str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)
