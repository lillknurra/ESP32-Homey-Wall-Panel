#!/usr/bin/env python3
import pathlib
import os
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="panel-wifi-txn-") as tmp:
    binary = pathlib.Path(tmp) / "test_panel_wifi_transaction"
    command = [
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(ROOT / "include"),
        str(ROOT / "panel_wifi_transaction.c"),
        str(ROOT / "test_host" / "test_panel_wifi_transaction.c"),
    ]
    if os.environ.get("Q2_HOST_SANITIZERS") == "1":
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    command += ["-o", str(binary)]
    subprocess.run(command, check=True)
    subprocess.run([str(binary)], check=True)
