from pathlib import Path
import subprocess
import tempfile

component = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as td:
    binary = Path(td) / "liveness"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(component / "include"),
        str(component / "panel_homey_boot_liveness.c"),
        str(component / "test_host/test_panel_homey_boot_liveness.c"),
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("Patch117H bounded bootstrap liveness tests: PASS")
