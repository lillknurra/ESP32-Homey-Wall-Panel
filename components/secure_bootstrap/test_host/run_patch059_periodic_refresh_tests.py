#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
RUNTIME = ROOT / "components" / "secure_bootstrap" / "athom_oauth_runtime.c"
TEMPLATE = ROOT / "components" / "secure_bootstrap" / "test_host" / "test_patch059_periodic_refresh.c"


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


runtime = RUNTIME.read_text(encoding="utf-8")
template = TEMPLATE.read_text(encoding="utf-8")
assert re.search(r"#define PERIODIC_REFRESH_INTERVAL_MS 60000ULL", runtime)
assert re.search(r"#define PERIODIC_REFRESH_BUSY_DEFER_MS 5000ULL", runtime)
assert re.search(r"#define PERIODIC_REFRESH_COOLDOWN_MS 30000ULL", runtime)
assert "athom_cloud_inspect_device_snapshot(now_ms, &inspection)" in function_body(
    runtime, "static void periodic_inventory_refresh_scheduler("
)
assert "queue_inventory_refresh_if_ready(\n                    ATHOM_REFRESH_ORIGIN_PERIODIC)" in runtime
worker = function_body(runtime, "static void homey_command_worker(")
assert "inventory_refresh_worker_should_retry_after_cloud_429(" in worker
assert "command.origin == ATHOM_REFRESH_ORIGIN_BOOT_AUTO" in worker

marker = "/* PATCH059_PRODUCTION_FUNCTIONS */"
assert template.count(marker) == 1
production = "\n\n".join(
    function_body(runtime, signature)
    for signature in (
        "static bool inventory_refresh_worker_should_retry(",
        "static bool inventory_refresh_worker_should_retry_after_cloud_429(",
        "static bool periodic_refresh_scheduler_should_attempt(",
        "static uint64_t periodic_refresh_scheduler_defer_ms(",
        "static void periodic_refresh_scheduler_record_queue_result(",
        "static athom_refresh_queue_result_t queue_inventory_refresh_if_ready(",
    )
)
generated = template.replace(marker, production)

with tempfile.TemporaryDirectory() as temporary_directory:
    temporary = Path(temporary_directory)
    source = temporary / "test_patch059_periodic_refresh.c"
    binary = temporary / "test_patch059_periodic_refresh"
    source.write_text(generated, encoding="utf-8")
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
         str(source), "-o", str(binary)],
        check=True,
        cwd=ROOT,
    )
    subprocess.run([str(binary)], check=True, cwd=ROOT)
print("PATCH059_PERIODIC_REFRESH_TESTS PASS")
