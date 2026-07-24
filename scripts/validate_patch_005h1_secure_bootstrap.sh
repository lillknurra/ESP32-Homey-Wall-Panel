#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/p005h1b.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

awk 'NF { print }' config/patch_005_scope_manifest.txt | sort -u > "$TMP/expected"
{
  git diff --name-only
  git ls-files --others --exclude-standard
} | awk 'NF && $0 != "dependencies.lock" { print }' | sort -u > "$TMP/actual"
diff -u "$TMP/expected" "$TMP/actual" || fail "source scope"
test "$(awk 'NF { n++ } END { print n+0 }' config/patch_005_scope_manifest.txt)" = "39" || fail "scope count"
! grep -Fxq dependencies.lock config/patch_005_scope_manifest.txt || fail "generated lock in source scope"

python3 - <<'PY'
from pathlib import Path
for rel in ('scripts/run_patch_005h1_hardware_bootstrap.py',):
    source = Path(rel).read_text(encoding='utf-8')
    compile(source, rel, 'exec')
PY
bash -n scripts/validate_patch_005h1_secure_bootstrap.sh

python3 - <<'PYTEST'
import builtins
import importlib.util
from pathlib import Path

path = Path("scripts/run_patch_005h1_hardware_bootstrap.py")
spec = importlib.util.spec_from_file_location("hardware_runner", path)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)
checks = {}
original = builtins.input
try:
    builtins.input = lambda prompt="": "no"
    try:
        module.require_confirm(checks, "bootstrap_text_readable", "readable?")
        raise AssertionError("mandatory no did not abort")
    except module.MandatoryCheckFailed as error:
        assert error.check_name == "bootstrap_text_readable"
        assert checks["bootstrap_text_readable"] is False
finally:
    builtins.input = original
print("PASS: hardware runner mandatory-fail behavior")
PYTEST

cc -std=c11 -Wall -Wextra -Werror \
  -Icomponents/secure_bootstrap/include \
  components/secure_bootstrap/secure_bootstrap_logic.c \
  components/secure_bootstrap/test_host/test_secure_bootstrap.c \
  -o "$TMP/bootstrap"
"$TMP/bootstrap"

# Prior Athom host functionality, deliberately without historical scope gates.
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_transport_contract.c \
  -o "$TMP/transport-contract"
"$TMP/transport-contract"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_runtime.c \
  components/athom_cloud_native/test_host/test_executable_protocol.c \
  -o "$TMP/executable-protocol"
"$TMP/executable-protocol"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_live_transport.c \
  -o "$TMP/live-transport"
"$TMP/live-transport"

# Bootstrap runtime closure wiring.
grep -Fq 'waveshare/esp32_s3_touch_lcd_4b: "2.0.0"' main/idf_component.yml || fail "waveshare BSP pin"
grep -Fq 'bsp_display_start' components/secure_bootstrap/secure_bootstrap_esp.c || fail "display wiring"
grep -Fq '&lv_font_montserrat_28' components/secure_bootstrap/secure_bootstrap_esp.c || fail "28px UI font wiring"
grep -Fq '&lv_font_montserrat_22' components/secure_bootstrap/secure_bootstrap_esp.c || fail "22px UI font wiring"
grep -Fq 'CONFIG_LV_FONT_MONTSERRAT_28=y' sdkconfig.defaults || fail "28px font config"
grep -Fq 'CONFIG_LV_FONT_MONTSERRAT_22=y' sdkconfig.defaults || fail "22px font config"
grep -Fq 'lv_qrcode_update' components/secure_bootstrap/secure_bootstrap_esp.c || fail "QR wiring"
grep -Fq 'CONFIG_LV_USE_QRCODE=y' sdkconfig.defaults || fail "QR config"
grep -Fq 'esp_fill_random' components/secure_bootstrap/secure_bootstrap_esp.c || fail "random source"
grep -Fq 'secure_bootstrap_code_rotation_due' components/secure_bootstrap/secure_bootstrap_esp.c || fail "runtime rotation"
grep -Fq 'esp_wifi_deauth_sta(0)' components/secure_bootstrap/secure_bootstrap_esp.c || fail "old SoftAP session invalidation"
grep -Fq 'httpd_stop' components/secure_bootstrap/secure_bootstrap_esp.c || fail "HTTP close"
grep -Fq 'esp_wifi_set_mode(WIFI_MODE_STA)' components/secure_bootstrap/secure_bootstrap_esp.c || fail "SoftAP close"
grep -Fq 'BOOTSTRAP_CLOSED softap=false http=false code_wiped=true physical_reopen_required=true ip_obtained=true' components/secure_bootstrap/secure_bootstrap_esp.c || fail "closure marker"
grep -Fq 'WIFI_AUTH_WPA2_PSK' components/secure_bootstrap/secure_bootstrap_esp.c || fail "WPA2"
! grep -Fq 'WIFI_AUTH_OPEN' components/secure_bootstrap/secure_bootstrap_esp.c || fail "open AP"
grep -Fq 'SECURE_BOOTSTRAP_WIPE_HOLD_MS 5000' components/secure_bootstrap/include/secure_bootstrap.h || fail "wipe threshold"
grep -Fq 'esp_wifi_restore' components/secure_bootstrap/secure_bootstrap_esp.c || fail "Wi-Fi reset"
! grep -Fq 'nvs_flash_erase' components/secure_bootstrap/secure_bootstrap_esp.c || fail "global NVS erase"
grep -Fq 'oauth_locked=true' components/secure_bootstrap/secure_bootstrap_esp.c || fail "OAuth lock"
grep -Fq 'Athom OAuth and Athom credential portal are disabled' main/main.c || fail "Athom disabled marker"
! grep -Fq 'athom_provisioning_portal_start' main/main.c || fail "Athom portal active"
! grep -Fq 'Type OAUTH' scripts/run_patch_005h1_hardware_bootstrap.py || fail "OAuth confirmation"
! grep -Eq '(^|[^[:alnum:]_])timeout[[:space:]]' scripts/run_patch_005h1_hardware_bootstrap.py || fail "GNU timeout"
grep -Fq '"/bin/bash", "-c"' scripts/run_patch_005h1_hardware_bootstrap.py || fail "shell mode"
grep -Fq 'wrong_password_rejected' scripts/run_patch_005h1_hardware_bootstrap.py || fail "wrong password hardware check"
grep -Fq 'http_endpoint_closed' scripts/run_patch_005h1_hardware_bootstrap.py || fail "HTTP closure hardware check"
grep -Fq 'bootstrap_text_readable' scripts/run_patch_005h1_hardware_bootstrap.py || fail "text legibility hardware check"
grep -Fq 'MandatoryCheckFailed' scripts/run_patch_005h1_hardware_bootstrap.py || fail "mandatory fail-fast"
grep -Fq 'Partial evidence' scripts/run_patch_005h1_hardware_bootstrap.py || fail "partial evidence on fail"


# Patch 005H.1F deterministic layout, physical reprovision and evidence redaction.
grep -Fq '#define CODE_CAPTION_Y 134' components/secure_bootstrap/secure_bootstrap_esp.c || fail "code caption coordinate"
grep -Fq '#define CODE_Y 168' components/secure_bootstrap/secure_bootstrap_esp.c || fail "code coordinate"
grep -Fq '#define CODE_CAPTION_TO_CODE_GAP 12' components/secure_bootstrap/secure_bootstrap_esp.c || fail "code separation gap"
grep -Fq 's_code_caption = lv_label_create(screen)' components/secure_bootstrap/secure_bootstrap_esp.c || fail "separate code caption object"
grep -Fq 'BOOTSTRAP_PHYSICAL_REPROVISION hold_ms=5000 accepted=true' components/secure_bootstrap/secure_bootstrap_esp.c || fail "physical reprovision marker"
grep -Fq 'BOOTSTRAP_RESTARTING reason=physical_reprovision' components/secure_bootstrap/secure_bootstrap_esp.c || fail "physical restart marker"
grep -Fq 'BOOT_BUTTON_GPIO GPIO_NUM_0' components/secure_bootstrap/secure_bootstrap_esp.c || fail "BOOT GPIO0"
grep -Fq '.pull_up_en = GPIO_PULLUP_ENABLE' components/secure_bootstrap/secure_bootstrap_esp.c || fail "BOOT internal pull-up"
grep -Fq 'gpio_get_level(BOOT_BUTTON_GPIO) == 0' components/secure_bootstrap/secure_bootstrap_esp.c || fail "BOOT active-low"
grep -Fq 'code_label_separated' scripts/run_patch_005h1_hardware_bootstrap.py || fail "code separation hardware check"
grep -Fq 'fresh_bootstrap_visible' scripts/run_patch_005h1_hardware_bootstrap.py || fail "fresh bootstrap hardware check"
grep -Fq 'PHYSICAL_MARKER' scripts/run_patch_005h1_hardware_bootstrap.py || fail "physical marker capture"
grep -Fq 'RESTART_MARKER' scripts/run_patch_005h1_hardware_bootstrap.py || fail "restart marker capture"

python3 - <<'PYREDACT'
import importlib.util
from pathlib import Path
p = Path('scripts/run_patch_005h1_hardware_bootstrap.py')
spec = importlib.util.spec_from_file_location('runner', p)
m = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(m)
fixture = (
    'wifi:connected with MyPrivateNetwork, aid = 5\n'
    'wifi:mode : sta (aa:bb:cc:dd:ee:ff) + softAP (11:22:33:44:55:66)\n'
    'esp_netif_handlers: sta ip: 192.168.1.50, mask: 255.255.255.0, gw: 192.168.1.1\n'
    'assigned IP to a client, IP is: 192.168.4.2\n'
)
out = m.sanitize(fixture)
assert 'MyPrivateNetwork' not in out
assert 'aa:bb:cc:dd:ee:ff' not in out
assert '192.168.1.50' not in out
assert not any(m.redaction_status(out).values())
print('PASS: 005H.1F evidence redaction fixtures')
PYREDACT

grep -Fq 'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y' sdkconfig.defaults || fail "flash size"
grep -Fq 'CONFIG_SPIRAM_MODE_OCT=y' sdkconfig.defaults || fail "octal PSRAM"
grep -Fq '# CONFIG_SPIRAM_MODE_QUAD is not set' sdkconfig.defaults || fail "quad PSRAM disabled"
grep -Fq 'CONFIG_SPIRAM_SPEED_80M=y' sdkconfig.defaults || fail "PSRAM 80 MHz"
grep -Fq 'CONFIG_SPIRAM_BOOT_INIT=y' sdkconfig.defaults || fail "PSRAM boot init"
grep -Fq 'factory,app,factory,0x10000,0x600000' partitions.csv || fail "partition size"
! grep -Fq 'CONFIG_NVS_ENCRYPTION=y' sdkconfig.defaults || fail "NVS encryption"

test -s dependencies.lock || fail "generated dependencies.lock missing"
if git check-ignore -q dependencies.lock; then :; else fail "generated lock must be ignored in model B"; fi
if git check-ignore -q managed_components && git check-ignore -q build; then :; else fail "generated directories not ignored"; fi

npm --prefix tools/homey-inventory ci
npm --prefix tools/homey-inventory test
bash scripts/validate_patch_004.sh

! grep -R -n -E 'setCapabilityValue|triggerFlow|startFlow|activateMood|genericApiCall' components/secure_bootstrap main || fail "mutation"
! grep -R -n -E 'client_secret[[:space:]]*=[[:space:]]*"[^" ]+|access_token[[:space:]]*=[[:space:]]*"[^" ]+|refresh_token[[:space:]]*=[[:space:]]*"[^" ]+|authorization_code[[:space:]]*=[[:space:]]*"[^" ]+' components/secure_bootstrap main scripts docs/history/PATCH_005H1_SECURE_LOCAL_HARDWARE_BOOTSTRAP.md || fail "Athom secret"

git diff --check
printf 'PASS: Patch 005H.1F layout, physical reprovision and evidence redaction validation\n'
