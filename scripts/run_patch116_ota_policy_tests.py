#!/usr/bin/env python3
"""Build/run Patch116 pure C policy regression on a host, no ESP-IDF or device."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
TEST = ROOT / "components/secure_bootstrap/test_host/test_patch116_ota_policy.c"
CODE = ROOT / "components/secure_bootstrap/panel_ota_policy.c"
INCLUDE = ROOT / "components/secure_bootstrap/include"


def main() -> int:
    compiler = os.environ.get("CC") or shutil.which("cc")
    if not compiler:
        print("PATCH116_POLICY=NOT_RUN compiler not available", file=sys.stderr)
        return 2
    with tempfile.TemporaryDirectory(prefix="patch116-host-policy-") as directory:
        binary = Path(directory) / "policy_test"
        build = [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-pedantic", "-O2", "-I", str(INCLUDE), str(TEST), str(CODE),
                 "-o", str(binary)]
        subprocess.run(build, check=True)
        subprocess.run([str(binary)], check=True)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
