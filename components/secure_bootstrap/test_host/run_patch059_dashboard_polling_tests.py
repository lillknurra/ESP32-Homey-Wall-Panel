#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
SOURCE = COMPONENT / "secure_bootstrap_esp.c"
TEMPLATE = COMPONENT / "test_host" / "test_patch059_dashboard_polling.c"


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing production function: {signature}")
    brace = source.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing function body: {signature}")
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


source = SOURCE.read_text(encoding="utf-8")
poll = function_body(source, "static void poll_homey_dashboard_if_due(")
assert "dashboard_snapshot_poll_allowed(" in poll
assert "s_panel_dashboard_visible" in poll
assert "athom_cloud_copy_device_snapshot(" in poll
assert "queue_inventory_refresh" not in poll
assert "dashboard_favorites_apply_allowed(runtime_state)" in poll
assert "dashboard_poll_requires_refresh(model_changed, favorites_changed)" in poll

template = TEMPLATE.read_text(encoding="utf-8")
marker = "/* PATCH059_PRODUCTION_FUNCTIONS */"
assert template.count(marker) == 1
production = "\n\n".join(
    function_body(source, signature)
    for signature in (
        "static bool dashboard_snapshot_poll_allowed(",
        "static bool dashboard_favorites_apply_allowed(",
        "static bool dashboard_poll_requires_refresh(",
    )
)
generated = template.replace(marker, production)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    c_source = temporary / "test_patch059_dashboard_polling.c"
    binary = temporary / "test_patch059_dashboard_polling"
    c_source.write_text(generated, encoding="utf-8")
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
         "-I", str(COMPONENT / "include"),
         str(COMPONENT / "panel_homey_dashboard_binding.c"),
         str(c_source), "-o", str(binary)],
        check=True,
        cwd=ROOT,
    )
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_DASHBOARD_POLLING_TESTS PASS")
