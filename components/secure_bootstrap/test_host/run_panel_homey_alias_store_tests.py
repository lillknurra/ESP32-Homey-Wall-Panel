#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
    output = pathlib.Path(temp) / "test_panel_homey_alias_store"
    command = [
        "cc", "-std=c11", "-D_POSIX_C_SOURCE=200809L",
        "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(root / "include"),
        str(root / "panel_homey_alias_store.c"),
        str(root / "panel_homey_dashboard_binding.c"),
        str(root / "test_host/test_panel_homey_alias_store.c"),
        "-o", str(output),
    ]
    print("ALIAS_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True)
    subprocess.run([str(output)], check=True)
print("PATCH_015_ALIAS_HOST_RUNNER=PASS")
