from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[3]
COMPONENT = ROOT / "components" / "secure_bootstrap"
JOURNAL = (COMPONENT / "runtime_diag_journal.c").read_text(encoding="utf-8")
RUNTIME = (COMPONENT / "athom_oauth_runtime.c").read_text(encoding="utf-8")
ESP = (COMPONENT / "secure_bootstrap_esp.c").read_text(encoding="utf-8")
PHONE = (COMPONENT / "phone_provisioning_store.c").read_text(encoding="utf-8")
MAIN = (ROOT / "main" / "main.c").read_text(encoding="utf-8")
NVS_GATE = (ROOT / "main" / "nvs_startup_gate.c").read_text(encoding="utf-8")
HEADER = (COMPONENT / "include" / "runtime_diag_journal.h").read_text(encoding="utf-8")
PARTITIONS = (ROOT / "partitions.csv").read_text(encoding="utf-8").splitlines()


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    assert start >= 0, f"missing function: {signature}"
    brace = source.find("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += source[index] == "{"
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated function: {signature}")


assert "diaglog,data,undefined,0x610000,0x100000," in PARTITIONS
assert "nvs,data,nvs,0x9000,0x6000," in PARTITIONS
assert "phy_init,data,phy,0xf000,0x1000," in PARTITIONS
assert "factory,app,factory,0x10000,0x600000," in PARTITIONS
assert len([line for line in PARTITIONS if line.startswith("diaglog,")]) == 1
assert "RUNTIME_DIAG_JOURNAL_RECORD_SIZE 64U" in HEADER
assert "RUNTIME_DIAG_JOURNAL_CAPACITY 16384U" in HEADER
assert "RUNTIME_DIAG_JOURNAL_PAGE_DEFAULT 64U" in HEADER
assert "RUNTIME_DIAG_JOURNAL_PAGE_MAX 256U" in HEADER
assert "esp_partition_erase_range" not in JOURNAL
assert "esp_flash_erase" not in JOURNAL
assert "nvs_" not in JOURNAL
assert "esp_partition_write" in JOURNAL
assert "esp_partition_read" in JOURNAL
assert "xQueueCreateStatic" in JOURNAL
assert re.search(r"xQueueSend\(s_event_queue,\s*event,\s*0U\)", JOURNAL)
assert "esp_partition_find_first(" in JOURNAL
assert "s_partition->size != DIAGLOG_SIZE_BYTES" in JOURNAL
assert "s_partition->address != 0x610000U" in JOURNAL
assert "DIAGLOG_COMMIT_OFFSET" in JOURNAL and "DIAGLOG_COMMIT_WORD" in JOURNAL

route_block = RUNTIME[RUNTIME.index("const httpd_uri_t handlers[]="):]
route_block = route_block[:route_block.index("};") + 2]
assert route_block.count("HTTP_GET") + route_block.count("HTTP_POST") == 10
assert route_block.count("/homey/debug/runtime-journal") == 1
assert '"/homey/debug/runtime-journal",HTTP_GET,runtime_diag_journal_get' in route_block
phone_routes = PHONE[PHONE.index("const httpd_uri_t u[]={"):]
phone_routes = phone_routes[:phone_routes.index("};") + 2]
assert phone_routes.count("HTTP_GET") + phone_routes.count("HTTP_POST") == 10
base_routes = function_body(ESP, "static esp_err_t server_start(void)\n{")
assert base_routes.count("httpd_register_uri_handler(s_server") == 4
assert 4 + 10 + 10 == 24
assert "cfg.max_uri_handlers=24" in ESP
assert "cfg.max_uri_handlers=23" not in ESP

endpoint = function_body(RUNTIME, "static esp_err_t runtime_diag_journal_get(")
for forbidden in (
    "queue_inventory_refresh_if_ready",
    "connect_and_fetch_inventory",
    "athom_cloud_",
    "network_phase_",
    "esp_partition_write",
    "esp_partition_erase",
    "nvs_",
    "runtime_diag_journal_record",
):
    assert forbidden not in endpoint, f"endpoint must remain passive: {forbidden}"
assert "runtime_diag_journal_get_page" in endpoint
assert "calloc(limit, sizeof(*records))" in endpoint
assert "httpd_resp_send_chunk" in endpoint
assert "runtime_diag_journal_parse_query" in function_body(
    RUNTIME, "static bool runtime_diag_parse_query("
)
assert "before_seq" in JOURNAL
assert "httpd_query_key_value" not in endpoint

boot = function_body(MAIN, "void app_main(void)")
assert boot.index("runtime_diag_journal_start") < boot.index("panel_nvs_startup_gate")
assert boot.index("runtime_diag_journal_record") < boot.index("panel_nvs_startup_gate")
assert "panel_nvs_startup_gate" in boot
assert re.search(r"if\s*\(!panel_nvs_startup_gate\(\)\)\s*\{\s*return;\s*\}", boot)
assert boot.index("panel_nvs_startup_gate") < boot.index("secure_bootstrap_start")
assert "nvs_flash_erase" not in boot
assert "nvs_flash_erase" not in NVS_GATE
assert "nvs_flash_init()" in NVS_GATE
assert "status != ESP_OK" in NVS_GATE
assert "return false" in NVS_GATE
assert "NVS initialization failed; normal startup halted" in NVS_GATE
assert "athom_runtime_start(" not in boot and "athom_runtime_start(" not in ESP

for private_or_variable_content in (
    "Authorization", "Bearer", "homey_id", "device_id", "capability_id",
    "response_body", "request_body", "ssid", "password",
):
    assert private_or_variable_content not in JOURNAL

print("PATCH060_RUNTIME_JOURNAL_INVARIANTS PASS")
