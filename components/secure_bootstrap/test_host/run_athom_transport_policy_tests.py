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
    temporary = Path(temporary_directory)
    binary = temporary / "test_athom_transport_policy"
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
    runtime_text = runtime_source.read_text(encoding="utf-8")
    protected_release = "owner != ATHOM_NETWORK_PHASE_NONE &&\n        s_network_phase_owner == owner"
    if runtime_text.count(protected_release) != 1:
        raise SystemExit("negative control could not locate the owner-matched release guard")
    mutated_runtime = temporary / "athom_oauth_runtime_without_owner_match.c"
    mutated_runtime.write_text(
        runtime_text.replace(protected_release,
                             "owner != ATHOM_NETWORK_PHASE_NONE &&\n        true", 1),
        encoding="utf-8",
    )
    negative = subprocess.run(
        [str(binary), str(client_source), str(client_header), str(mutated_runtime)],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if negative.returncode == 0:
        raise SystemExit("negative control failed to detect removal of owner matching")
    print("NETWORK_PHASE_OWNER_MATCH_NEGATIVE_CONTROL=DETECTED")
    print("ATHOM_TRANSPORT_POLICY_RUNNER PASS")
