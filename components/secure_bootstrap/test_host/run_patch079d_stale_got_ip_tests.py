#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / "components/secure_bootstrap"
BASE = Path.home() / "Downloads/PATCH079B_TARGETED_SAFETY_FIX_OFFLINE_20261014/source"
IDF = Path.home() / "GitHub/esp-idf-v6.0.1"
OUT = Path.home() / "Downloads/PATCH079D_STALE_GOT_IP_SAFETY_FIX_OFFLINE_20261014/test_logs"
OUT.mkdir(parents=True, exist_ok=True)
CC = "cc"
COMMON = [CC, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g",
          "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
          "-I", str(HERE / "include")]

def run(name, args, expected):
    binary = OUT / name
    subprocess.run(args + ["-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
    output = result.stdout.strip()
    if expected not in output:
        raise RuntimeError(f"{name}: expected {expected!r}, got {output!r}")
    (OUT / f"{name}.log").write_text(output + "\n")
    print(output)

logic = HERE / "secure_bootstrap_logic.c"
old_logic = BASE / "components/secure_bootstrap/secure_bootstrap_logic.c"
run("patch079d_stale_got_ip_before",
    COMMON + [str(HERE / "test_host/test_patch079d_stale_got_ip_before.c"), str(old_logic)],
    "EXPECTED_FAIL_BEFORE_FIX")
run("patch079d_stale_got_ip_after",
    COMMON + [str(HERE / "test_host/test_patch079d_stale_got_ip_gate.c"), str(logic)],
    "PASS: production Wi-Fi candidate GOT_IP gate interleavings")

for name, test, extra, expected in [
    ("patch079d_first_install_resubmit", "test_patch079b_first_install_resubmit.c", [],
     "PASS: first-install cancel rotates session and resubmission produces CONNECT"),
    ("patch079d_scan_snapshot", "test_patch079b_scan_snapshot.c", ["-pthread"],
     "PASS: production scan cancel fence and snapshot copy/publish"),
    ("patch079d_cancel_commit", "test_patch079_wifi_cancel_atomicity.c", [],
     "PASS: Patch079 Wi-Fi cancel/commit arbitration"),
]:
    run(name, COMMON + extra + [str(HERE / "test_host" / test), str(logic)], expected)

secure = (HERE / "secure_bootstrap_esp.c").read_text()
got = secure[secure.index("if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)"):]
got = got[:got.index("if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)")]
assert "wifi_got_ip_matches_candidate(got_ip)" in got
assert "secure_bootstrap_wifi_candidate_got_ip_verified(" in got
assert got.index("wifi_got_ip_matches_candidate(got_ip)") < got.index("SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP")
candidate_gate = got.index("secure_bootstrap_wifi_candidate_got_ip_verified(")
candidate_accept = got.index("if (!accepted)", candidate_gate)
candidate_online = got.index("s_ip_obtained = secure_bootstrap_sta_link_apply_event(", candidate_accept)
assert candidate_gate < candidate_accept < candidate_online
handler = secure[secure.index("static esp_err_t wifi_handler(httpd_req_t *req)"):]
handler = handler[:handler.index("static esp_err_t status_handler")]
assert handler.index("candidate_disconnect_and_drain_events(request_generation)") < handler.index("esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config)")
assert handler.index("esp_wifi_set_config(WIFI_IF_STA, &s_candidate_config)") < handler.index("secure_bootstrap_wifi_candidate_arm_connect(")
assert handler.index("secure_bootstrap_wifi_candidate_arm_connect(") < handler.index("apply_actions(candidate_actions)")
assert "esp_netif_action_disconnected(" in secure
assert "esp_event_post(SECURE_BOOTSTRAP_WIFI_FENCE_EVENT" in secure
assert "WIFI_EVENT_STA_DISCONNECTED" in secure
assert "s_candidate_connect_generation" in got
assert "password" not in got.lower() or "candidate.sta.password" not in got.lower()

idf_event = (IDF / "components/esp_event/esp_event.c").read_text()
assert "xQueueSendToBack(loop->queue, &post" in idf_event
assert "event_data_copy" in idf_event and "memcpy(event_data_copy, event_data, event_data_size)" in idf_event
netif = (IDF / "components/esp_netif/lwip/esp_netif_lwip.c").read_text()
assert "esp_netif_down_api" in netif and "dhcp_stop(esp_netif->lwip_netif)" in netif
handlers = (IDF / "components/esp_netif/esp_netif_handlers.c").read_text()
assert "void esp_netif_action_disconnected" in handlers and "esp_netif_down(esp_netif)" in handlers
print("PASS: production callback gates precede online/commit; candidate install follows disconnect/DHCP-down/FIFO fence")
