#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
test_source = root / "components/secure_bootstrap/test_host/test_athom_transport_policy.c"
client_source = root / "components/secure_bootstrap/athom_cloud_client.c"
client_header = root / "components/secure_bootstrap/include/athom_cloud_client.h"
runtime_source = root / "components/secure_bootstrap/athom_oauth_runtime.c"

with tempfile.TemporaryDirectory() as temporary_directory:
    binary = Path(temporary_directory) / "test_athom_transport_policy"
    command = [
        "cc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pedantic",
        str(test_source),
        "-o",
        str(binary),
    ]
    print("TRANSPORT_POLICY_HOST_COMPILE:", " ".join(command))
    subprocess.run(command, check=True)
    subprocess.run(
        [
            str(binary),
            str(client_source),
            str(client_header),
            str(runtime_source),
        ],
        check=True,
    )
    print("ATHOM_TRANSPORT_POLICY_RUNNER PASS")
