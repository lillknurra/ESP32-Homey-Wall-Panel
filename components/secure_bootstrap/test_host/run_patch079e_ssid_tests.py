#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = ROOT / "components/secure_bootstrap"
OUT = Path("/Users/petter/Downloads/PATCH082_WIFI_SETTINGS_OFFLINE_INTEGRATION_V1_20261014/test_runs/ssid")
OUT.mkdir(parents=True, exist_ok=True)
COMMON = ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-g",
          "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
          "-I", str(HERE / "include")]

def run(name, sources, expected):
    binary = OUT / name
    subprocess.run(COMMON + [str(x) for x in sources] + ["-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
    text = result.stdout.strip()
    if expected not in text:
        raise RuntimeError(f"{name}: expected {expected!r}, got {text!r}")
    (OUT / f"{name}.log").write_text(text + "\n")
    print(text)

run("patch079e_ssid_association",
    [HERE / "test_host/test_patch079e_ssid_association.c", HERE / "secure_bootstrap_logic.c"],
    "PASS: real production SSID comparator")

secure = (HERE / "secure_bootstrap_esp.c").read_text()
logic = (HERE / "secure_bootstrap_logic.c").read_text()
header = (HERE / "include/secure_bootstrap.h").read_text()
assert "secure_bootstrap_wifi_ssid_association_matches(" in secure
assert "secure_bootstrap_wifi_ssid_association_matches(" in logic
assert "sizeof(ap.ssid)" not in secure
assert "memcmp(ap.ssid, candidate.sta.ssid" not in secure
handler = secure[secure.index("static esp_err_t wifi_handler(httpd_req_t *req)"):]
handler = handler[:handler.index("static esp_err_t status_handler")]
assert "ssid_len > 32U" in handler
assert "memcpy(candidate_config.sta.ssid, ssid, ssid_len)" in handler
assert "snprintf(\n        (char *)candidate_config.sta.ssid" not in handler
assert secure.index("wifi_got_ip_matches_candidate(got_ip)") < secure.index("SECURE_BOOTSTRAP_STA_LINK_EVENT_GOT_IP")
assert "secure_bootstrap_wifi_candidate_got_ip_verified(" in secure
print("PASS: ESP integration invokes tested comparator before online/commit; 32-byte candidate input is bounded and copied length-safely")
