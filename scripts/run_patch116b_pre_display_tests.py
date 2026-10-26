#!/usr/bin/env python3
"""Host-only; does not invoke ESP-IDF, flash, Homey, or network."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
include = root / "components/secure_bootstrap/include"
sources = [
    root / "components/secure_bootstrap/panel_ota_pre_display_policy.c",
    root / "components/secure_bootstrap/test_host/test_patch116b_pre_display.c",
]
compilers = [name for name in ("clang", "gcc") if shutil.which(name)]
if not compilers:
    raise SystemExit("FAIL: no C compiler available")
for compiler in compilers:
    with tempfile.TemporaryDirectory(prefix="patch116b_host_") as directory:
        binary = Path(directory) / "test_policy"
        cmd = [compiler, "-std=c11", "-O1", "-g", "-Wall", "-Wextra",
               "-Werror", "-pedantic", "-I", str(include)]
        # Sanitizer availability differs between macOS clang and Linux GCC.
        # Native sanitizer runs are separately attempted without pretending
        # that an unsupported sanitizer means the test ran under it.
        cmd += [str(p) for p in sources] + ["-o", str(binary)]
        subprocess.run(cmd, check=True)
        output = subprocess.check_output([str(binary)], text=True)
        if "PATCH116B_PREDISPLAY_POLICY_HOST_TESTS=PASS cases=24" not in output:
            raise SystemExit(f"FAIL: unexpected test result from {compiler}: {output}")
        print(f"{compiler}: {output.strip()}")
        sanitizer = cmd.copy()
        sanitizer[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        sanitized_binary = Path(directory) / "test_policy_sanitized"
        sanitizer[sanitizer.index("-o")+1] = str(sanitized_binary)
        try:
            subprocess.run(sanitizer, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        except subprocess.CalledProcessError:
            print(f"{compiler}: sanitizers NOT_AVAILABLE (ordinary host tests PASS)")
        else:
            env = dict(os.environ)
            env["ASAN_OPTIONS"] = "detect_leaks=0"
            output = subprocess.check_output([str(sanitized_binary)], text=True, env=env)
            if "PATCH116B_PREDISPLAY_POLICY_HOST_TESTS=PASS cases=24" not in output:
                raise SystemExit(f"FAIL: unexpected sanitizer result from {compiler}: {output}")
            print(f"{compiler}: ASAN_UBSAN=PASS")
print("PATCH116B_HOST_VALIDATION=PASS (not a firmware or hardware test)")
