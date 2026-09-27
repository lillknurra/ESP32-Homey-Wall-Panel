#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory() as td:
    binary = Path(td) / "test_panel_homey_awning_provisioning"
    command = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        "-I",
        str(root / "include"),
        str(root / "panel_homey_awning_provisioning.c"),
        str(Path(__file__).with_name("test_panel_homey_awning_provisioning.c")),
        "-o",
        str(binary),
    ]
    print("HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)

print("PATCH051_AWNING_PROVISIONING_RUNNER=PASS")
