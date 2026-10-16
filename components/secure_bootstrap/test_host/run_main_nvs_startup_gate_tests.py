from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[3]
GATE = ROOT / "main" / "nvs_startup_gate.c"

NVS_HEADER = """
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NVS_NO_FREE_PAGES 0x110d
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1110
esp_err_t nvs_flash_init(void);
"""

LOG_HEADER = """
#define ESP_LOGE(tag, format, ...) ((void)(tag))
"""

HARNESS = r"""
#include <assert.h>
#include "nvs_flash.h"
#include "nvs_startup_gate.h"

static esp_err_t next_status;
static unsigned int init_calls;
esp_err_t nvs_flash_init(void) { ++init_calls; return next_status; }

static void check_status(esp_err_t status, int expected)
{
    next_status = status;
    init_calls = 0;
    assert(panel_nvs_startup_gate() == expected);
    assert(init_calls == 1);
}

int main(void)
{
    check_status(ESP_OK, 1);
    check_status(ESP_ERR_NVS_NO_FREE_PAGES, 0);
    check_status(ESP_ERR_NVS_NEW_VERSION_FOUND, 0);
    check_status(0x1234, 0);
    return 0;
}
"""


with tempfile.TemporaryDirectory(prefix="panel-nvs-gate-test-") as temp:
    temp_path = Path(temp)
    (temp_path / "nvs_flash.h").write_text(NVS_HEADER, encoding="utf-8")
    (temp_path / "esp_log.h").write_text(LOG_HEADER, encoding="utf-8")
    (temp_path / "harness.c").write_text(HARNESS, encoding="utf-8")
    binary = temp_path / "test_nvs_gate"
    subprocess.run(
        ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
         "-I", str(temp_path), "-I", str(ROOT / "main"),
         str(GATE), str(temp_path / "harness.c"), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("MAIN_NVS_STARTUP_GATE PASS (OK and init-error cases; single init, no erase API)")
