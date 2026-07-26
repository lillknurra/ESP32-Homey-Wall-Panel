#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat PAGER=cat GH_PAGER=cat LESS=FRX
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
ALLOWED="components/secure_bootstrap/CMakeLists.txt
components/secure_bootstrap/include/secure_bootstrap.h
components/secure_bootstrap/secure_bootstrap_esp.c
components/secure_bootstrap/secure_bootstrap_logic.c
components/secure_bootstrap/homey_panel_font_22.c
components/secure_bootstrap/include/homey_panel_font_22.h
components/secure_bootstrap/test_host/test_secure_bootstrap.c
components/secure_bootstrap/test_host/test_wifi_state_machine.c
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/handoff/MASTER_INDEX.md
docs/history/PATCH_007_PERSISTENT_WIFI_RECONNECT_AND_NETWORK_SELECTION.md
docs/history/PATCH_HISTORY.md
main/main.c
scripts/validate_patch_007.sh"
REQUIRED="components/secure_bootstrap/include/secure_bootstrap.h
components/secure_bootstrap/secure_bootstrap_esp.c
components/secure_bootstrap/secure_bootstrap_logic.c
components/secure_bootstrap/test_host/test_secure_bootstrap.c
components/secure_bootstrap/test_host/test_wifi_state_machine.c
main/main.c
scripts/validate_patch_007.sh"
ACTUAL="$(git status --porcelain=v1 | sed -E 's/^.. //' | sort)"
UNKNOWN="$(comm -23 <(printf '%s\n' "$ACTUAL" | sed '/^$/d' | sort) <(printf '%s\n' "$ALLOWED" | sort))"
MISSING="$(comm -23 <(printf '%s\n' "$REQUIRED" | sort) <(printf '%s\n' "$ACTUAL" | sed '/^$/d' | sort))"
test -z "$UNKNOWN" || { echo "FAIL: files outside locked Patch 007 scope"; printf '%s\n' "$UNKNOWN"; exit 1; }
test -z "$MISSING" || { echo "FAIL: required final Patch 007 changes missing"; printf '%s\n' "$MISSING"; exit 1; }
echo "PASS: final Patch 007 files are contained in locked scope"
git diff --check
BUILD_DIR="${TMPDIR:-/tmp}/patch007-host-$$"; rm -rf "$BUILD_DIR"; mkdir -p "$BUILD_DIR"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/secure_bootstrap/include components/secure_bootstrap/secure_bootstrap_logic.c components/secure_bootstrap/test_host/test_secure_bootstrap.c -o "$BUILD_DIR/bootstrap_test"
"$BUILD_DIR/bootstrap_test"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/secure_bootstrap/include components/secure_bootstrap/secure_bootstrap_logic.c components/secure_bootstrap/test_host/test_wifi_state_machine.c -o "$BUILD_DIR/wifi_state_test"
"$BUILD_DIR/wifi_state_test"
rm -rf "$BUILD_DIR"
grep -q 'SECURE_BOOTSTRAP_WIFI_PERSISTING' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'SECURE_BOOTSTRAP_WIFI_PERSIST_VERIFY_CONNECTING' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'SECURE_BOOTSTRAP_WIFI_ROLLING_BACK' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'SECURE_BOOTSTRAP_WIFI_ACTION_START_PERSIST' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'SECURE_BOOTSTRAP_WIFI_BACKUP_MAGIC' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'payload_crc32' components/secure_bootstrap/include/secure_bootstrap.h
grep -q 'secure_bootstrap_wifi_backup_encode' components/secure_bootstrap/secure_bootstrap_logic.c
grep -q 'secure_bootstrap_wifi_backup_decode' components/secure_bootstrap/secure_bootstrap_logic.c
grep -q 'WIFI_STORAGE_RAM' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_PERSIST begin' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_PERSIST verify_valid=' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_PERSIST complete' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_ROLLBACK begin' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_BACKUP write_result=' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'WIFI_BACKUP clear_result=' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 's_rollback_attempted' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'persistent_commit_task' components/secure_bootstrap/secure_bootstrap_esp.c
! grep -q 'WIFI_REINIT_DIAG' components/secure_bootstrap/secure_bootstrap_esp.c
! grep -q 'NVS_SENTINEL' components/secure_bootstrap/secure_bootstrap_esp.c
! grep -q 'NVS_SENTINEL' main/main.c
! grep -q 'SECURE_BOOTSTRAP_WIFI_ACTION_COMMIT_CANDIDATE' components/secure_bootstrap/include/secure_bootstrap.h
# Exactly one physical-wipe restore remains; no restart in persistent or rollback helpers.
test "$(grep -n 'esp_wifi_restore' components/secure_bootstrap/secure_bootstrap_esp.c | wc -l | tr -d ' ')" = "1"
PERSIST_SECTION="$(sed -n '/static void persistent_commit_task/,/static esp_err_t schedule_persistent_commit/p' components/secure_bootstrap/secure_bootstrap_esp.c)"
! grep -q 'esp_restart' <<<"$PERSIST_SECTION"
# API-order anchors: stop/deinit precede first init/config; second deinit/init precedes verify get.
python3 - <<'PY'
from pathlib import Path
s=Path('components/secure_bootstrap/secure_bootstrap_esp.c').read_text()
fn=s[s.index('static esp_err_t persist_write_and_reopen'):s.index('static void persist_fail')]
anchors=['esp_wifi_stop()', 'esp_wifi_deinit()', 'driver_init_flash()', 'esp_wifi_set_config(', 'esp_wifi_deinit()', 'driver_init_flash()', 'esp_wifi_get_config(']
pos=-1
for a in anchors:
    pos=fn.find(a,pos+1)
    if pos < 0: raise SystemExit('FAIL: persistent API order anchor missing: '+a)
print('PASS: persistent API order verified')
PY
# Portal UTF-8, AP-only prefill, client/server validation and credential hygiene.
grep -q 'text/html; charset=utf-8' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q '<meta charset=\\"UTF-8\\">' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'request_uses_setup_ap' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'autocomplete=off value=' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'Välj ett nätverk' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'data-security=' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'Detta är ett öppet nätverk' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'type=submit disabled' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'new TextEncoder' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'find_scanned_network' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'password_len < 8U || password_len > 63U' components/secure_bootstrap/secure_bootstrap_esp.c
! grep -q 'name=code type=password required>' components/secure_bootstrap/secure_bootstrap_esp.c
! grep -Eq 'ESP_LOG[IEWD].*(code|password|ssid).*%s' components/secure_bootstrap/secure_bootstrap_esp.c
! git diff -- tools/homey-inventory components/athom_cloud_native | grep -q .
source "$HOME/GitHub/esp-idf-v6.0.1/export.sh" >/dev/null
idf.py set-target esp32s3 >/dev/null
idf.py build

python3 - <<'PY'
from pathlib import Path

source = Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")

required = [
    'if (code_present) {',
    'PORTAL_ROOT chunk=code skipped=true',
    'local.ss_family == AF_INET',
    'local.ss_family == AF_INET6',
    'PORTAL_ROUTE local_family=%d',
    'PORTAL_ROUTE ipv4_mapped=%s',
    'address[10] == 0xffU && address[11] == 0xffU',
    'memcpy(&local_ipv4, &address[12], sizeof(local_ipv4));',
    'setup_ap_match = local_ipv4 == ap_ip.ip.addr',
    '<meta charset=\\"UTF-8\\">',
    'id=code',
    'id=ssid',
    'id=password',
    'id=connect',
]
missing = [needle for needle in required if needle not in source]
if missing:
    raise SystemExit("portal render/route validation missing: " + ", ".join(missing))

code_send = 'result = httpd_resp_sendstr_chunk(req, escaped_code);'
position = source.find(code_send)
guard = source.rfind('if (code_present) {', 0, position)
if position < 0 or guard < 0 or position - guard > 250:
    raise SystemExit("panel-code chunk is not guarded by code_present")

credential_log_fragments = [
    'PORTAL_ROUTE local_ip=',
    'PORTAL_ROUTE ap_ip=',
    'PORTAL_ROOT code=%',
    'PORTAL_ROOT ssid=%',
    'PORTAL_ROOT password=%',
]
present = [needle for needle in credential_log_fragments if needle in source]
if present:
    raise SystemExit("credential-bearing portal diagnostics found: " + ", ".join(present))

print("PASS: portal render and AP route validation")
PY


python3 - <<'PY'
from pathlib import Path

source = Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")

bad = """networkOk=s.value!==''&&(!manual||bytes(ms.value)>0&&bytes(ms.value)<=32),"
        "m.hidden=!manual;"""
good = """networkOk=s.value!==''&&(!manual||bytes(ms.value)>0&&bytes(ms.value)<=32);"
        "m.hidden=!manual;"""

if bad in source:
    raise SystemExit("portal JavaScript still contains invalid const-declaration separator")
if source.count(good) != 1:
    raise SystemExit("portal JavaScript corrected validation/manual-toggle sequence missing or duplicated")

required = [
    "e.addEventListener('input',validate)",
    "e.addEventListener('change',validate)",
    "validate();",
    "s.value==='__manual__'",
    "b.disabled=!",
]
missing = [needle for needle in required if needle not in source]
if missing:
    raise SystemExit("portal JavaScript validation wiring missing: " + ", ".join(missing))

print("PASS: portal JavaScript syntax guard")
PY


python3 - <<'PY'
from pathlib import Path

header = Path("components/secure_bootstrap/include/secure_bootstrap.h").read_text(encoding="utf-8")
logic = Path("components/secure_bootstrap/secure_bootstrap_logic.c").read_text(encoding="utf-8")
esp = Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")
test = Path("components/secure_bootstrap/test_host/test_wifi_state_machine.c").read_text(encoding="utf-8")

required_header = [
    "SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE",
    "SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED",
]
required_logic = [
    "context->state = SECURE_BOOTSTRAP_WIFI_PROVISIONING;",
    "SECURE_BOOTSTRAP_WIFI_ACTION_OPEN_PROVISIONING",
    "SECURE_BOOTSTRAP_WIFI_EVENT_PROVISIONING_OPEN_FAILED",
    "SECURE_BOOTSTRAP_WIFI_ACTION_RESTORE_SAVED |",
    "SECURE_BOOTSTRAP_WIFI_ACTION_CONNECT",
]
required_esp = [
    'lv_label_set_text(touch_label, "Ändra Wi-Fi");',
    "LV_EVENT_CLICKED",
    "s_reconfigure_pending",
    'xTaskCreate(',
    '"wifi_reconfigure"',
    "SECURE_BOOTSTRAP_WIFI_EVENT_USER_RECONFIGURE",
    "WIFI_RECONFIGURE requested=true",
    "set_reconfigure_button_visible(true)",
    "set_reconfigure_button_visible(false)",
    "secure_zero(&s_candidate_config, sizeof(s_candidate_config));",
]
required_test = [
    "test_online_user_reconfigure",
    "test_reconfigure_open_failure_returns_online",
    "test_candidate_retry_exhaustion_restores_saved",
]

for name, text, required in [
    ("header", header, required_header),
    ("logic", logic, required_logic),
    ("esp", esp, required_esp),
    ("test", test, required_test),
]:
    missing = [needle for needle in required if needle not in text]
    if missing:
        raise SystemExit(f"reconfigure validation missing in {name}: " + ", ".join(missing))

for forbidden in [
    'lv_label_set_text(touch_label, "Touch test");',
    "BOOTSTRAP_TOUCH_EVENT",
]:
    if forbidden in esp:
        raise SystemExit("obsolete touch-test behavior remains: " + forbidden)

credential_fragments = [
    "WIFI_RECONFIGURE ssid=",
    "WIFI_RECONFIGURE password=",
    "WIFI_RECONFIGURE code=",
]
if any(fragment in esp for fragment in credential_fragments):
    raise SystemExit("credential-bearing reconfigure logging found")

print("PASS: Patch 007 reconfigure button and rollback guards")
PY


python3 - <<'PY'
from pathlib import Path

esp = Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")
cmake = Path("components/secure_bootstrap/CMakeLists.txt").read_text(encoding="utf-8")
font_c = Path("components/secure_bootstrap/homey_panel_font_22.c").read_text(encoding="utf-8")
font_h = Path("components/secure_bootstrap/include/homey_panel_font_22.h").read_text(encoding="utf-8")

required_esp = [
    '#include "homey_panel_font_22.h"',
    '&homey_panel_font_22',
    'lv_label_set_text(touch_label, "Ändra Wi-Fi");',
    '<button id=connect type=submit disabled>Anslut panelen till valt nätverk</button>',
    'button{white-space:normal;line-height:1.25;min-height:3.2rem}',
    'send_candidate_status_page(',
    '<!doctype html><html lang=\\"sv\\">',
    '<meta charset=\\"UTF-8\\">',
    'name=\\"viewport\\"',
    'font:18px system-ui',
    '<h1>Nätverket testas</h1>',
    'misslyckas återställs "',
    '"den tidigare Wi-Fi-konfigurationen automatiskt.',
    'misslyckas kan du "',
    '"försöka igen från panelens installationsläge',
    'return send_candidate_status_page(req, s_wifi.saved_config_present);',
    'b.disabled=!',
]
missing = [item for item in required_esp if item not in esp]
if missing:
    raise SystemExit("UI/UX runtime validation missing: " + ", ".join(missing))

if '&lv_font_montserrat_22' in esp[esp.find('lv_label_set_text(touch_label'):esp.find('lv_obj_center(touch_label)')]:
    raise SystemExit("touch_label still uses lv_font_montserrat_22")
if '<button id=connect type=submit disabled>Anslut</button>' in esp:
    raise SystemExit("old standalone submit text remains")
if 'Nätverket testas. Återgå till panelen.' in esp:
    raise SystemExit("raw candidate status response remains")

if '"homey_panel_font_22.c"' not in cmake:
    raise SystemExit("font source missing from CMakeLists.txt")
if 'LV_FONT_DECLARE(homey_panel_font_22)' not in font_h:
    raise SystemExit("font declaration missing")
for cp in ["0x00a4", "0x00a5", "0x00b6", "0x00c4", "0x00c5", "0x00d6"]:
    if cp not in font_c.lower():
        raise SystemExit("font cmap missing offset " + cp)
if '.range_start = 0x20' not in font_c or '.bpp = 4' not in font_c:
    raise SystemExit("font metadata missing")
if '.list_length = 101' not in font_c:
    raise SystemExit("font glyph coverage count is incorrect")

status_start = esp.find("static esp_err_t send_candidate_status_page")
status_end = esp.find("static esp_err_t wifi_handler", status_start)
status = esp[status_start:status_end]
for forbidden in ["ssid", "password", "panelkod", "bssid", "candidate_config", "s_candidate_config"]:
    if forbidden.lower() in status.lower():
        raise SystemExit("credential-bearing status-page token found: " + forbidden)

print("PASS: Patch 007 UI/UX font, submit label and status page")
PY


python3 - <<'PY'
from pathlib import Path

source = Path("components/secure_bootstrap/secure_bootstrap_esp.c").read_text(encoding="utf-8")

required = [
    '"Säker Wi-Fi-installation"',
    '"Anslut till HomeyPanel-Setup\\nÖppna 192.168.4.1"',
    '"Anslut till HomeyPanel-Setup\\nÖppna 192.168.4.1"',
    '"Panelkod (%02lld:%02lld):"',
    'format_code_caption(',
    'refresh_code_countdown();',
    's_code.issued_at_s + SECURE_BOOTSTRAP_CODE_TTL_SECONDS - current_s',
    'lv_obj_set_style_text_font(s_title, &homey_panel_font_22, 0);',
    'lv_obj_set_style_text_font(s_detail, &homey_panel_font_22, 0);',
    'lv_obj_set_style_text_font(s_code_caption, &homey_panel_font_22, 0);',
]
missing = [needle for needle in required if needle not in source]
if missing:
    raise SystemExit("Swedish setup display validation missing: " + ", ".join(missing))

forbidden = [
    '"Secure Wi-Fi setup"',
    '"Connect to HomeyPanel-Setup\\nOpen 192.168.4.1"',
    '"Code rotated\\nConnect to HomeyPanel-Setup\\nOpen 192.168.4.1"',
    '"Panelkoden har förnyats',
    'bootstrap_visible ? "Code:" : ""',
]
present = [needle for needle in forbidden if needle in source]
if present:
    raise SystemExit("English setup display text remains: " + ", ".join(present))

for unchanged in [
    'lv_obj_set_style_text_font(s_code_label, &lv_font_montserrat_28, 0);',
    '#define QR_SIZE 180',
    'lv_qrcode_set_size(s_qr, QR_SIZE);',
    'lv_obj_align(s_qr, LV_ALIGN_BOTTOM_MID, 0, -QR_BOTTOM_MARGIN);',
]:
    if unchanged not in source:
        raise SystemExit("Unrelated setup display behavior changed: " + unchanged)

print("PASS: Swedish local setup display with code countdown")
PY

printf 'PASS: final Patch 007 host and ESP-IDF validation\n'
