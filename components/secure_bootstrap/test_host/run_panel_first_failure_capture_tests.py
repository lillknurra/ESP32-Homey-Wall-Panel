#!/usr/bin/env python3
"""Compile the actual first-failure worker helper with bounded host doubles."""
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[3]
component = root / "components/secure_bootstrap"
runtime = (component / "athom_oauth_runtime.c").read_text()


def extract_function(signature: str) -> str:
    start = runtime.index(signature)
    brace = runtime.index("{", start)
    depth = 0
    for pos in range(brace, len(runtime)):
        if runtime[pos] == "{":
            depth += 1
        elif runtime[pos] == "}":
            depth -= 1
            if depth == 0:
                return runtime[start : pos + 1]
    raise AssertionError(f"unterminated function: {signature}")


production = "\n\n".join(
    [
        extract_function("static uint8_t first_boot_stage_code("),
        extract_function("static void first_failure_capture_attempt("),
    ]
)
fixture_path = component / "test_host/test_panel_first_failure_capture.c"
fixture = fixture_path.read_text().replace(
    "/* PATCH119P4N12_PRODUCTION_FUNCTIONS */", production
)
assert "PATCH119P4N12_PRODUCTION_FUNCTIONS" not in fixture

with tempfile.TemporaryDirectory(prefix="first-failure-capture-") as tmp:
    tmp_path = pathlib.Path(tmp)
    generated = tmp_path / "production_capture_test.c"
    exe = tmp_path / "production_capture_test"
    generated.write_text(fixture)
    subprocess.run(
        [
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-pthread",
            "-I",
            str(component / "include"),
            str(component / "panel_first_failure_diag.c"),
            str(generated),
            "-o",
            str(exe),
        ],
        check=True,
    )
    result = subprocess.run([str(exe)], check=True, text=True, capture_output=True)
    assert result.stdout.strip() == "PRODUCTION_FIRST_FAILURE_CAPTURE=PASS"
    print(result.stdout.strip())
