#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile


COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]

with tempfile.TemporaryDirectory() as temporary_directory:
    binary = Path(temporary_directory) / "test_runtime_diag_journal"
    subprocess.run(
        [
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-DRUNTIME_DIAG_JOURNAL_HOST_TEST",
            "-I", str(COMPONENT / "include"),
            str(COMPONENT / "runtime_diag_journal.c"),
            str(COMPONENT / "test_host" / "test_runtime_diag_journal.c"),
            "-o", str(binary),
        ],
        check=True,
        cwd=ROOT,
    )
    subprocess.run([str(binary)], check=True, cwd=ROOT)

print("RUNTIME_DIAG_JOURNAL_HOST_RUNNER PASS")
