#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
test_source = Path(__file__).with_name("test_panel_ui_model.c")

with tempfile.TemporaryDirectory() as temporary_directory:
    binary = Path(temporary_directory) / "test_panel_ui_model"
    command = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-I",
        str(root / "include"),
        str(root / "panel_ui_model.c"),
        str(root / "panel_ui_store.c"),
        str(test_source),
        "-o",
        str(binary),
    ]
    print("HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)
    print("PANEL_UI_HOST_RUNNER PASS")
