from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
JOURNAL = (COMPONENT / "runtime_diag_journal.c").read_text(encoding="utf-8")
HEADER = (COMPONENT / "include" / "runtime_diag_journal.h").read_text(
    encoding="utf-8"
)
ESP = (COMPONENT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")


def body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start >= 0, f"missing function: {signature}"
    brace = source.find("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


assert "runtime_diag_journal_pause_flash_writes(void)" in HEADER
pause = body(JOURNAL, "esp_err_t runtime_diag_journal_pause_flash_writes(void)")
assert pause.index("xSemaphoreTake(s_media_mutex, portMAX_DELAY)") < pause.index(
    "s_flash_writes_paused = true"
)
assert pause.index("s_flash_writes_paused = true") < pause.index(
    "xSemaphoreGive(s_media_mutex)"
)

append = body(JOURNAL, "static esp_err_t append_event(")
mutex_take = append.index("xSemaphoreTake(s_media_mutex, portMAX_DELAY)")
gate_check = append.index("const bool writes_paused = s_flash_writes_paused")
write_call = append.index("append_core(partition_append_read")
assert mutex_take < gate_check < write_call
assert "if (writes_paused) {" in append[gate_check:write_call]
assert "xSemaphoreGive(s_media_mutex)" in append[gate_check:write_call]

record = body(JOURNAL, "bool runtime_diag_journal_record(")
assert "s_flash_writes_paused" in record
assert "xQueueCreateStatic(DIAGLOG_QUEUE_DEPTH" in JOURNAL
assert "esp_partition_erase_range" not in JOURNAL
assert "esp_flash_erase" not in JOURNAL

display_init = body(ESP, "static esp_err_t display_init(void)")
assert display_init.index("runtime_diag_journal_pause_flash_writes()") < display_init.index(
    "bsp_display_start()"
)
assert "ESP_RETURN_ON_ERROR(runtime_diag_journal_pause_flash_writes()" in display_init

print("PATCH109_JOURNAL_RGB_FLASH_GATE PASS")
