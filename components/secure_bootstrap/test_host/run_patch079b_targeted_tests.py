#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[3]
BASE = Path.home() / "Downloads/PATCH079_SAFE_WIFI_DIALOG_CANCEL_V2/source"
HERE = ROOT / "components/secure_bootstrap"
parser = argparse.ArgumentParser()
parser.add_argument("--output-dir", type=Path, required=True)
OUT = parser.parse_args().output_dir
OUT.mkdir(parents=True, exist_ok=True)
CC = "cc"
COMMON = [CC, "-std=c11", "-Wall", "-Wextra", "-Werror", "-g",
          "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
          "-I", str(HERE / "include")]
test = HERE / "test_host/test_patch079b_first_install_resubmit.c"
logic = HERE / "secure_bootstrap_logic.c"
old_logic = BASE / "components/secure_bootstrap/secure_bootstrap_logic.c"

def run(name, cmd, expected=None):
    binary = OUT / name
    subprocess.run(cmd + ["-o", str(binary)], check=True)
    result = subprocess.run([str(binary)], check=True, capture_output=True, text=True)
    text = result.stdout.strip()
    if expected is not None:
        assert expected in text, text
    (OUT / f"{name}.log").write_text(text + "\n")
    print(text)

run("patch079b_r2_before", COMMON + ["-DPATCH079B_PRE_FIX", str(test), str(old_logic)],
    "EXPECTED_FAIL_BEFORE_FIX")
run("patch079b_r2_after", COMMON + [str(test), str(logic)],
    "PASS: first-install cancel rotates session and resubmission produces CONNECT")
run("patch079b_scan_snapshot", COMMON + ["-pthread",
    str(HERE / "test_host/test_patch079b_scan_snapshot.c"), str(logic)],
    "PASS: production scan cancel fence and snapshot copy/publish")

# Production integration is checked against the actual ESP orchestration, while
# state/session transitions and scan snapshot operations above execute production C.
secure = (HERE / "secure_bootstrap_esp.c").read_text()
ui = (HERE / "panel_ui.c").read_text()
cancel = secure[secure.index("static void wifi_cancel_task(void *arg)"):]
cancel = cancel[:cancel.index("cancel_failed:")]
assert "secure_bootstrap_wifi_cancel_first_install(\n        &s_wifi, &s_wifi_session, generation)" in cancel
first_install = cancel[cancel.index("/* First-install cancellation retains"):]
assert first_install.index("secure_bootstrap_wifi_cancel_first_install") < first_install.index("if (!kept)")
handler = secure[secure.index("static esp_err_t wifi_handler(httpd_req_t *req)"):]
handler = handler[:handler.index("static esp_err_t status_handler")]
assert "SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT" in handler
assert "const esp_err_t connect_result = apply_actions(candidate_actions);" in handler
assert "if (connect_result != ESP_OK)" in handler
apply = secure[secure.index("static esp_err_t apply_actions(uint32_t actions)", secure.index("static esp_err_t apply_actions(uint32_t actions)")+1):]
apply = apply[:apply.index("static ", 1)]
assert "connect_current()" in apply
assert "esp_wifi_connect()" in secure
assert "xSemaphoreTake(s_networks_mutex" in secure
assert "secure_bootstrap_network_snapshot_copy" in secure
assert "secure_bootstrap_network_snapshot_publish" in secure
assert "const secure_bootstrap_network_t *scanned" not in secure
assert "secure_bootstrap_scan_cancel(&s_scan_state)" in cancel
assert "secure_bootstrap_scan_begin(&s_scan_state, session_generation)" in secure
assert "secure_bootstrap_scan_complete(" in secure
assert "secure_bootstrap_scan_complete(\n            &s_scan_state, scan_generation, current_generation, current_session)" in secure
assert "if (!published)" in secure
assert "lv_obj_add_flag(qr, LV_OBJ_FLAG_HIDDEN)" in ui
assert 'set_label_text_if_changed(ui->wifi_password, "")' in ui
assert 'status_modal_show(ui, MODAL_ERROR, "Fel"' in ui
print("PASS: ESP orchestration connects through CONNECT action; scan and QR fail-closed wiring present")
