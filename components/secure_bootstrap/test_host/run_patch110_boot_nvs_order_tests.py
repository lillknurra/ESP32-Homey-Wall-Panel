from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[3]
BOOT = (ROOT / "main" / "main.c").read_text(encoding="utf-8")
SOURCE = (ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")
BASE = subprocess.check_output(
    ["git", "show", "b80785fc16f1547bf11371810f5df47265b0a851:components/secure_bootstrap/secure_bootstrap_esp.c"],
    cwd=ROOT, text=True,
)


def function_body(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[opening + 1:end - 1]


startup = function_body(SOURCE, "secure_bootstrap_status_t secure_bootstrap_start(void)")
assert BOOT.index("panel_nvs_startup_gate()") < BOOT.index("secure_bootstrap_start()")
assert startup.index("legacy_sentinel_clear();") < startup.index("display_init()")
assert startup.count("legacy_sentinel_clear();") == 1
assert SOURCE.count("legacy_sentinel_clear();") == BASE.count("legacy_sentinel_clear();") == 3
assert function_body(SOURCE, "static void legacy_sentinel_clear(void)") == function_body(
    BASE, "static void legacy_sentinel_clear(void)"
)
assert "runtime_diag_journal_pause_flash_writes()" in function_body(
    SOURCE, "static esp_err_t display_init(void)"
)
assert "runtime_diag_journal_pause_flash_writes()" in function_body(
    BASE, "static esp_err_t display_init(void)"
)
print("PATCH110_NVS_BEFORE_DISPLAY_ORDER PASS")
