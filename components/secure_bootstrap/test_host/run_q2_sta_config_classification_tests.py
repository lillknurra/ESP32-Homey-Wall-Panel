#!/usr/bin/env python3
"""Compile the actual STA identity/empty helpers against a deterministic ABI fixture."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SOURCE = (ROOT / "components/secure_bootstrap/secure_bootstrap_esp.c").read_text()


def extract(signature: str) -> str:
    start = SOURCE.index(signature)
    brace = SOURCE.index("{", start + len(signature))
    depth = 0
    for index in range(brace, len(SOURCE)):
        if SOURCE[index] == "{":
            depth += 1
        elif SOURCE[index] == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[start:index + 1]
    raise AssertionError(f"unterminated: {signature}")


ABI = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct { int8_t rssi; uint8_t authmode; } wifi_scan_threshold_t;
typedef struct { bool capable; bool required; } wifi_pmf_config_t;
typedef struct {
    uint8_t ssid[32], password[64];
    uint32_t scan_method;
    bool bssid_set;
    uint8_t bssid[6], channel;
    uint16_t listen_interval;
    uint32_t sort_method;
    wifi_scan_threshold_t threshold;
    wifi_pmf_config_t pmf_cfg;
    uint32_t rm_enabled:1, btm_enabled:1, mbo_enabled:1, ft_enabled:1;
    uint32_t owe_enabled:1, transition_disable:1;
    uint32_t disable_wpa3_compatible_mode:1, reserved1:25;
    uint32_t sae_pwe_h2e, sae_pk_mode;
    uint8_t failure_retry_cnt;
    uint32_t he_dcm_set:1, he_dcm_max_constellation_tx:2;
    uint32_t he_dcm_max_constellation_rx:2, he_mcs9_enabled:1;
    uint32_t he_su_beamformee_disabled:1;
    uint32_t he_trig_su_bmforming_feedback_disabled:1;
    uint32_t he_trig_mu_bmforming_partial_feedback_disabled:1;
    uint32_t he_trig_cqi_feedback_disabled:1;
    uint32_t vht_su_beamformee_disabled:1, vht_mu_beamformee_disabled:1;
    uint32_t vht_mcs8_enabled:1, reserved2:19;
    uint8_t sae_h2e_identifier[32];
} wifi_sta_config_t;
typedef struct { wifi_sta_config_t sta; } wifi_config_t;
"""

TEST = r"""
int main(void) {
    wifi_config_t cfg = {0};
    assert(wifi_sta_config_empty(&cfg));
    cfg.sta.password[0] = 'x';
    assert(!wifi_sta_config_empty(&cfg));
    memset(&cfg, 0, sizeof(cfg)); cfg.sta.bssid[5] = 1U;
    assert(!wifi_sta_config_empty(&cfg));
    memset(&cfg, 0, sizeof(cfg)); cfg.sta.reserved1 = 1U;
    assert(!wifi_sta_config_empty(&cfg));
    memset(&cfg, 0, sizeof(cfg)); cfg.sta.reserved2 = 1U;
    assert(!wifi_sta_config_empty(&cfg));
    memset(&cfg, 0, sizeof(cfg)); cfg.sta.ssid[0] = 'n';
    assert(!wifi_sta_config_empty(&cfg));
    puts("PASS: actual Q2 STA empty-config classification");
    return 0;
}
"""

functions = "\n\n".join((
    extract("static bool wifi_sta_config_equal("),
    extract("static bool wifi_sta_config_empty("),
))
with tempfile.TemporaryDirectory(prefix="q2-sta-config-") as temp:
    source = Path(temp) / "test_sta_config.c"
    binary = Path(temp) / "test_sta_config"
    source.write_text(ABI + functions + TEST)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
