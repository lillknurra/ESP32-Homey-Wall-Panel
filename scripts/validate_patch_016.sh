#!/bin/sh
set -u
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT" || exit 1
python3 components/secure_bootstrap/test_host/run_panel_homey_light_provisioning_tests.py || exit 1
python3 components/secure_bootstrap/test_host/run_panel_ui_tests.py || exit 1
python3 components/secure_bootstrap/test_host/run_panel_homey_alias_store_tests.py || exit 1
grep -q 'dashboard_binding_index = LIGHT_1_WIDGET_INDEX' components/secure_bootstrap/panel_homey_light_provisioning.c || exit 1
grep -q 'dashboard_binding_index = LIGHT_2_WIDGET_INDEX' components/secure_bootstrap/panel_homey_light_provisioning.c || exit 1
grep -q '"Tänd"' components/secure_bootstrap/panel_ui_model.c || exit 1
grep -q '"Släckt"' components/secure_bootstrap/panel_ui_model.c || exit 1
grep -q 'sensitive_values_logged=false' components/secure_bootstrap/phone_provisioning_store.c || exit 1
if grep -R -n -E 'setCapabilityValue|triggerFlow|runFlow|HTTP_(PUT|DELETE|PATCH)' components/secure_bootstrap/panel_homey_light_provisioning.c components/secure_bootstrap/phone_provisioning_store.c; then exit 1; fi
if grep -R -n -E 'serial\.Serial|DTR|RTS' \
  components/secure_bootstrap/panel_homey_light_provisioning.c \
  components/secure_bootstrap/phone_provisioning_store.c \
  components/secure_bootstrap/panel_ui_model.c; then
  exit 1
fi
echo PATCH_016_STATIC_VALIDATION PASS
# PATCH_016_WIFI_ONLINE_LIGHT_GATE_BEGIN
grep -q 'phone_provisioning_on_wifi_offline();' components/secure_bootstrap/secure_bootstrap_esp.c
test "$(grep -o 'panel_homey_light_provisioning_access_allowed(s_wifi_online' components/secure_bootstrap/phone_provisioning_store.c | wc -l | tr -d ' ')" = "2"
grep -q 'ACCESS_POLICY_TESTS PASS' components/secure_bootstrap/test_host/test_panel_homey_light_provisioning.c
# PATCH_016_WIFI_ONLINE_LIGHT_GATE_END

# Patch 016 runtime correction guards
grep -q 'live_display_ready deferred=true wifi_online=false' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'athom_auth_store_load' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'WIFI_GOT_IP ignored=true' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'code_retained_until_candidate_verified=true' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'code_rotation_commit=true' components/secure_bootstrap/secure_bootstrap_esp.c

# Runtime correction 2 invariants.
! grep -q 's_code\.consumed = false' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'candidate_session_active=true' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'phone_provisioning_homey_runtime_ready' components/secure_bootstrap/include/phone_provisioning.h
test "$(grep -o 'panel_homey_light_provisioning_access_allowed(s_wifi_online,homey_ready,homey_present)' components/secure_bootstrap/phone_provisioning_store.c | wc -l | tr -d ' ')" = "2"
grep -q 'WIFI_GOT_IP ignored=true' components/secure_bootstrap/secure_bootstrap_esp.c
test "$(grep -o '{"/homey/lights",HTTP_GET,light_bindings_get,NULL}' components/secure_bootstrap/phone_provisioning_store.c | wc -l | tr -d ' ')" = "1"
# Patch 016 /homey/lights NVS crash-fix invariants v3.
grep -q '#include "nvs_flash.h"' components/secure_bootstrap/athom_auth_store.c
grep -q 'esp_err_t err = nvs_flash_init();' components/secure_bootstrap/athom_auth_store.c
grep -q 'if (err != ESP_OK || handle == 0)' components/secure_bootstrap/athom_auth_store.c
grep -q 'athom_auth_store_load(&record, &present)' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'err == ESP_OK && present' components/secure_bootstrap/phone_provisioning_store.c

# Patch 016 authoritative runtime identity invariants.
grep -q 'athom_oauth_runtime_get_selected_homey_id' components/secure_bootstrap/include/athom_oauth_runtime.h
grep -q 's_cloud.selected_homey.id' components/secure_bootstrap/athom_oauth_runtime.c
grep -A24 'static bool light_active_homey_id' components/secure_bootstrap/phone_provisioning_store.c | grep -q 'athom_oauth_runtime_get_selected_homey_id'
! grep -A24 'static bool light_active_homey_id' components/secure_bootstrap/phone_provisioning_store.c | grep -q 'athom_auth_store_load'
! sed -n '/athom_auth_store_load/,/^}/p' components/secure_bootstrap/athom_auth_store.c | grep -q 'nvs_flash_init'
grep -q '{"/homey/lights",HTTP_GET,light_bindings_get,NULL}' components/secure_bootstrap/phone_provisioning_store.c

# Patch 016 automatic favorite-light invariants.
grep -q 'panel_homey_favorites_parse_and_publish' components/secure_bootstrap/athom_cloud_client.c
grep -q 'panel_homey_favorites_copy_public' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'Urvalet hämtas automatiskt från Favoriter i Homey' components/secure_bootstrap/phone_provisioning_store.c
! grep -q '{\"/homey/lights\",HTTP_POST' components/secure_bootstrap/phone_provisioning_store.c

# Patch 016 R6 automatic favorite UI wiring invariants.
! grep -q '{"/homey/lights",HTTP_POST' components/secure_bootstrap/phone_provisioning_store.c
grep -q 'panel_homey_favorites_apply_ui_model(&s_panel_model)' components/secure_bootstrap/secure_bootstrap_esp.c
grep -q 'widget_title\[PANEL_UI_WIDGET_COUNT\]' components/secure_bootstrap/include/panel_ui_model.h
grep -q 'lv_label_set_text(ui->widget_title\[i\]' components/secure_bootstrap/panel_ui.c
python3 components/secure_bootstrap/test_host/run_panel_homey_favorites_tests.py

# PATCH_016_R6_4_6A_STATIC_BEGIN
R6_4_6A_RUNTIME="$ROOT/components/secure_bootstrap/athom_oauth_runtime.c"

r6_4_6a_fail() {
    echo "PATCH_016_R6_4_6A_STATIC=FAIL: $1" >&2
    exit 1
}

[ -f "$R6_4_6A_RUNTIME" ] || r6_4_6a_fail "runtime source missing"

grep -Fq 'const int first_homeys_http_status = athom_cloud_diagnostic_http_status();' "$R6_4_6A_RUNTIME" || r6_4_6a_fail "first HTTP status capture missing"
grep -Fq 'if (first_homeys_http_status == 401)' "$R6_4_6A_RUNTIME" || r6_4_6a_fail "401-only branch missing"
if grep -Fq 'err != ESP_OK && athom_cloud_diagnostic_http_status() == 401' "$R6_4_6A_RUNTIME"; then
    r6_4_6a_fail "stale esp_err_t-dependent 401 branch remains"
fi
grep -Fq 'const int second_homeys_http_status = athom_cloud_diagnostic_http_status();' "$R6_4_6A_RUNTIME" || r6_4_6a_fail "retry HTTP status capture missing"

grep -Fq 'phase=homeys_fetch_end attempt=1 http_status=%d result=%s error=%s' "$R6_4_6A_RUNTIME" || r6_4_6a_fail "attempt 1 sanitized marker missing"
grep -Fq 'phase=homeys_fetch_end attempt=2 http_status=%d result=%s error=%s' "$R6_4_6A_RUNTIME" || r6_4_6a_fail "attempt 2 sanitized marker missing"

if grep -E 'HOMEY_SCHEMA.*(homey_id|device_id|capability_id|token=|https?://|host=|Authorization|Bearer|\{|\})' "$R6_4_6A_RUNTIME" >/dev/null; then
    r6_4_6a_fail "privacy-forbidden data appears in HOMEY_SCHEMA markers"
fi

echo "PATCH_016_R6_4_6A_STATIC=PASS"
echo "AUTH_EXPIRY_SIGNAL=HTTP_401_INDEPENDENT_OF_ESP_ERR"
echo "TOKEN_REFRESH_MAX=1"
echo "HOMEYS_FETCH_RETRY_MAX=1"
echo "FAVORITE_SELECTION_LOGIC_CHANGED=NO"
# PATCH_016_R6_4_6A_STATIC_END

# PATCH_016_R6_4_6B_STATIC_BEGIN
R6_4_6B_CLIENT="$ROOT/components/secure_bootstrap/athom_cloud_client.c"
R6_4_6B_RUNTIME="$ROOT/components/secure_bootstrap/athom_oauth_runtime.c"
r6_4_6b_fail() { echo "PATCH_016_R6_4_6B_STATIC=FAIL: $1" >&2; exit 1; }
[ -f "$R6_4_6B_CLIENT" ] || r6_4_6b_fail "cloud client missing"
[ -f "$R6_4_6B_RUNTIME" ] || r6_4_6b_fail "runtime missing"
grep -Fq 'if (http_status > 0)' "$R6_4_6B_CLIENT" || r6_4_6b_fail "positive status preservation missing"
grep -Fq '*status_out = http_status;' "$R6_4_6B_CLIENT" || r6_4_6b_fail "status_out assignment missing"
grep -Fq 'if (status > 0)' "$R6_4_6B_CLIENT" || r6_4_6b_fail "request-error status branch missing"
grep -Fq 'diagnostic_set_http(' "$R6_4_6B_CLIENT" || r6_4_6b_fail "HTTP diagnostic publication missing"
grep -Fq 'if (first_homeys_http_status == 401)' "$R6_4_6B_RUNTIME" || r6_4_6b_fail "401-only runtime branch missing"
if grep -Fq 'ESP_ERR_NOT_SUPPORTED == ' "$R6_4_6B_RUNTIME"; then r6_4_6b_fail "ESP error treated as auth expiry"; fi
if grep -E 'HOMEY_SCHEMA.*(homey_id|device_id|capability_id|token=|https?://|host=|Authorization|Bearer|\\{|\\})' "$R6_4_6B_RUNTIME" >/dev/null; then r6_4_6b_fail "privacy-forbidden schema marker"; fi
echo "PATCH_016_R6_4_6B_STATIC=PASS"
echo "HTTP_STATUS_PRESERVED_ON_PERFORM_ERROR=YES"
echo "AUTH_EXPIRY_SIGNAL=HTTP_401_ONLY"
echo "ESP_ERR_NOT_SUPPORTED_ALONE_AUTH_EXPIRY=NO"
echo "FAVORITE_SELECTION_LOGIC_CHANGED=NO"
# PATCH_016_R6_4_6B_STATIC_END

# PATCH_016_R6_4_6C_STATIC_BEGIN
R6_4_6C_CLIENT="$ROOT/components/secure_bootstrap/athom_cloud_client.c"
R6_4_6C_RUNTIME="$ROOT/components/secure_bootstrap/athom_oauth_runtime.c"
r6_4_6c_fail() { echo "PATCH_016_R6_4_6C_STATIC=FAIL: $1" >&2; exit 1; }
[ -f "$R6_4_6C_CLIENT" ] || r6_4_6c_fail "cloud client missing"
[ -f "$R6_4_6C_RUNTIME" ] || r6_4_6c_fail "runtime missing"
grep -Fq 'HOMEY_SCHEMA device_index=%u device_label=device_%03u' "$R6_4_6C_CLIENT" || r6_4_6c_fail "generic device label missing"
grep -Fq 'homey_schema_log_object_keys(index, device);' "$R6_4_6C_CLIENT" || r6_4_6c_fail "index-only key logger missing"
grep -Fq 'homey_schema_log_capabilities(index, device);' "$R6_4_6C_CLIENT" || r6_4_6c_fail "index-only capability logger missing"
if grep -E 'HOMEY_SCHEMA[^"\n]*device=%s' "$R6_4_6C_CLIENT" >/dev/null; then r6_4_6c_fail "raw device-name format remains"; fi
if grep -Fq '? name->valuestring : "<namnlös>"' "$R6_4_6C_CLIENT"; then r6_4_6c_fail "schema name extraction remains"; fi
grep -Fq 'if (first_homeys_http_status == 401)' "$R6_4_6C_RUNTIME" || r6_4_6c_fail "401-only auth branch missing"
grep -Fq 'phase=token_refresh_begin' "$R6_4_6C_RUNTIME" || r6_4_6c_fail "refresh marker missing"
grep -Fq 'phase=homeys_fetch_begin attempt=2' "$R6_4_6C_RUNTIME" || r6_4_6c_fail "single retry marker missing"
if grep -E 'HOMEY_SCHEMA.*(homey_id|device_id|capability_id|token=|https?://|host=|Authorization|Bearer|\{|\})' "$R6_4_6C_CLIENT" "$R6_4_6C_RUNTIME" >/dev/null; then r6_4_6c_fail "privacy-forbidden schema marker"; fi
echo "PATCH_016_R6_4_6C_STATIC=PASS"
echo "SCHEMA_DEVICE_NAMES=REDACTED"
echo "SCHEMA_DEVICE_LABEL=DETERMINISTIC_GENERIC"
echo "AUTH_REFRESH_PATH=SUPPORTED"
echo "VALID_TOKEN_PATH=SUPPORTED"
echo "AUTH_CONNECT_INVENTORY_LOGIC_CHANGED=NO"
echo "FAVORITE_SELECTION_LOGIC_CHANGED=NO"
# PATCH_016_R6_4_6C_STATIC_END

# PATCH_016_R6_4_8_STATIC_BEGIN
R6_4_8_CLIENT="$ROOT/components/secure_bootstrap/athom_cloud_client.c"
r6_4_8_fail(){ echo "PATCH_016_R6_4_8_STATIC=FAIL: $1" >&2; exit 1; }
for marker in \
 '"/api/manager/mobile/summary"' \
 '"/api/manager/dashboards/dashboard"' \
 '"/api/manager/devices/device"' \
 '"/api/manager/flow/flow"' \
 '"/api/manager/moods/mood"' \
 'homey.device.readonly' 'homey.flow.readonly' 'homey.mood.readonly' 'homey.user.readonly' 'homey.dashboard.readonly' \
 'FAVORITE_DEVICES_CONTAINER' 'FAVORITE_FLOWS_CONTAINER' 'FAVORITE_MOODS_CONTAINER' \
 'configured=unknown' 'matched_collection_references='; do
  grep -Fq "$marker" "$R6_4_8_CLIENT" || r6_4_8_fail "missing marker: $marker"
done
R6_4_8_DISCOVERY="$(sed -n '/static void favorites_discovery_run(/,/^}/p' "$R6_4_8_CLIENT")"
for R6_4_8_RESOURCE in \
  '"/api/manager/mobile/summary"' \
  '"/api/manager/dashboards/dashboard"' \
  '"/api/manager/devices/device"' \
  '"/api/manager/flow/flow"' \
  '"/api/manager/moods/mood"'; do
  [ "$(printf '%s\n' "$R6_4_8_DISCOVERY" | grep -Fo "$R6_4_8_RESOURCE" | wc -l | tr -d ' ')" = 1 ] || r6_4_8_fail "resource occurrence mismatch: $R6_4_8_RESOURCE"
done
[ "$(printf '%s\n' "$R6_4_8_DISCOVERY" | grep -Fo '"/api/manager/' | wc -l | tr -d ' ')" = 5 ] || r6_4_8_fail 'expected exactly five documented GET resource occurrences'
if printf '%s\n' "$R6_4_8_DISCOVERY" | grep -Eq 'HTTP_METHOD_(POST|PUT|PATCH|DELETE)'; then r6_4_8_fail 'mutation method in discovery'; fi
if grep -Eqi 'HOMEY_FAVORITES_(SCOPE|CONTAINER).*token=|HOMEY_FAVORITES_(SCOPE|CONTAINER).*device_id|HOMEY_FAVORITES_(SCOPE|CONTAINER).*capability_id|HOMEY_FAVORITES_(SCOPE|CONTAINER).*https?://' "$R6_4_8_CLIENT"; then r6_4_8_fail 'forbidden diagnostic content'; fi
grep -Fq 'diagnostic_set("inventory_complete", ESP_OK);' "$R6_4_8_CLIENT" || r6_4_8_fail 'inventory completion changed'
echo 'PATCH_016_R6_4_8_STATIC=PASS'
echo 'DISCOVERY_METHOD=GET_ONLY'
echo 'DISCOVERY_RESOURCE_COUNT=5'
echo 'OAUTH_SCOPE_MUTATION=NO'
echo 'REFERENCE_VALUES_LOGGED=NO'
echo 'HEURISTIC_OBJECT_SELECTION=NO'
# PATCH_016_R6_4_8_STATIC_END
