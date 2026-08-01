#!/bin/sh

BASE="7782ba329689490cbe5b78ba8be0298a6f046dc3"
CHECKPOINT="0fe0656d841b28888dc6402af465c96e31e29e09"
BRANCH="patch-012-multi-page-dashboard-configuration-ui"
REMOTE_REF="origin/patch-012-multi-page-dashboard-configuration-ui"
MODE="${1:-production}"
FAIL=0

TEMP_SOURCE_SHA="88f6a4ac26ba2446e9a420ea0e97f9901bd0973d68c3be8a7f4e6e5ff8264fe0"
TEMP_HEADER_SHA="a740775326fb8baabca0342eb247d8b7438cf714f588ac33f0bd2cc919fda24d"
ORIG_SOURCE_SHA="0f3bbe997b4055620e93edc64b9628c5c5f8160ff6275367d8570dc40fa1ec4e"
ORIG_HEADER_SHA="c8ab9361f50a5fd2eecc442db58b9a2b69431ffbd3961b44ee77cd81e51f9668"
WAVE_SOURCE="managed_components/waveshare__esp32_s3_touch_lcd_4b/esp32_s3_touch_lcd_4b.c"
WAVE_HEADER="managed_components/waveshare__esp32_s3_touch_lcd_4b/include/bsp/esp32_s3_touch_lcd_4b.h"

fail() {
  echo "FAIL: $1"
  FAIL=1
}

working_paths() {
  {
    git diff --name-only
    git diff --cached --name-only
    git ls-files --others --exclude-standard
  } | sed '/^$/d' | sort -u
}

full_patch_paths() {
  {
    git diff --name-only origin/main...HEAD
    working_paths
  } | sed '/^$/d' | sort -u
}

case "$MODE" in
  contract|production) ;;
  *) echo "usage: $0 contract|production"; exit 2 ;;
esac

echo "PATCH_012_VALIDATION_MODE=$MODE"
[ "$(git branch --show-current)" = "$BRANCH" ] || fail "wrong branch"
[ "$(git rev-parse HEAD)" = "$CHECKPOINT" ] || fail "unexpected HEAD"
[ "$(git rev-parse "$REMOTE_REF")" = "$CHECKPOINT" ] || fail "unexpected remote feature branch"
[ "$(git rev-parse main)" = "$BASE" ] || fail "unexpected local main"
[ "$(git rev-parse origin/main)" = "$BASE" ] || fail "unexpected origin/main"
[ "$(git merge-base HEAD "$BASE")" = "$BASE" ] || fail "wrong merge base"

STAGED_COUNT="$(git diff --cached --name-only | sed '/^$/d' | wc -l | tr -d ' ')"
[ "$STAGED_COUNT" = "0" ] || fail "expected zero staged files, got $STAGED_COUNT"

WORKING="$(working_paths)"
WORKING_COUNT="$(printf '%s\n' "$WORKING" | sed '/^$/d' | wc -l | tr -d ' ')"
[ "$WORKING_COUNT" = "20" ] || fail "expected 20 Package 3A contract working-tree files, got $WORKING_COUNT"

for FILE in $WORKING; do
  case "$FILE" in
    components/secure_bootstrap/CMakeLists.txt|\
    components/secure_bootstrap/homey_panel_font_16.c|\
    components/secure_bootstrap/homey_panel_font_18.c|\
    components/secure_bootstrap/include/homey_panel_font_16.h|\
    components/secure_bootstrap/include/homey_panel_font_18.h|\
    components/secure_bootstrap/include/panel_ui.h|\
    components/secure_bootstrap/include/panel_ui_model.h|\
    components/secure_bootstrap/panel_ui.c|\
    components/secure_bootstrap/panel_ui_model.c|\
    components/secure_bootstrap/secure_bootstrap_esp.c|\
    components/secure_bootstrap/test_host/test_panel_ui_model.c|\
    docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|\
    docs/handoff/CURRENT_STATE.md|\
    docs/handoff/HANDOFF.md|\
    docs/handoff/MASTER_INDEX.md|\
    docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md|\
    docs/history/PATCH_HISTORY.md|\
    scripts/validate_patch_012.sh|\
    sdkconfig.defaults|\
    tools/serial_monitor.py) ;;
    *) fail "unexpected Package 3A working-tree file: $FILE" ;;
  esac
done

FULL="$(full_patch_paths)"
FULL_COUNT="$(printf '%s\n' "$FULL" | sed '/^$/d' | wc -l | tr -d ' ')"
[ "$FULL_COUNT" = "23" ] || fail "expected 23 full Patch 012 files, got $FULL_COUNT"

for FILE in $FULL; do
  case "$FILE" in
    components/secure_bootstrap/CMakeLists.txt|\
    components/secure_bootstrap/homey_panel_font_16.c|\
    components/secure_bootstrap/homey_panel_font_18.c|\
    components/secure_bootstrap/include/homey_panel_font_16.h|\
    components/secure_bootstrap/include/homey_panel_font_18.h|\
    components/secure_bootstrap/include/panel_ui.h|\
    components/secure_bootstrap/include/panel_ui_model.h|\
    components/secure_bootstrap/include/panel_ui_store.h|\
    components/secure_bootstrap/panel_ui.c|\
    components/secure_bootstrap/panel_ui_model.c|\
    components/secure_bootstrap/panel_ui_store.c|\
    components/secure_bootstrap/secure_bootstrap_esp.c|\
    components/secure_bootstrap/test_host/run_panel_ui_tests.py|\
    components/secure_bootstrap/test_host/test_panel_ui_model.c|\
    docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|\
    docs/handoff/CURRENT_STATE.md|\
    docs/handoff/HANDOFF.md|\
    docs/handoff/MASTER_INDEX.md|\
    docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md|\
    docs/history/PATCH_HISTORY.md|\
    scripts/validate_patch_012.sh|\
    sdkconfig.defaults|\
    tools/serial_monitor.py) ;;
    *) fail "unexpected full Patch 012 file: $FILE" ;;
  esac
done

for FILE in \
  components/secure_bootstrap/include/secure_bootstrap.h \
  components/secure_bootstrap/include/phone_provisioning.h \
  components/secure_bootstrap/phone_provisioning_store.c
do
  printf '%s\n' "$FULL" | grep -Fxq "$FILE" && fail "forbidden changed file: $FILE"
done

SOURCE_SHA="$(shasum -a 256 "$WAVE_SOURCE" | awk '{print $1}')"
HEADER_SHA="$(shasum -a 256 "$WAVE_HEADER" | awk '{print $1}')"
if [ "$MODE" = "contract" ]; then
  [ "$SOURCE_SHA" = "$TEMP_SOURCE_SHA" ] || fail "unexpected temporary Waveshare source hash"
  [ "$HEADER_SHA" = "$TEMP_HEADER_SHA" ] || fail "unexpected temporary Waveshare header hash"
else
  [ "$SOURCE_SHA" = "$ORIG_SOURCE_SHA" ] || fail "Waveshare source not restored for production"
  [ "$HEADER_SHA" = "$ORIG_HEADER_SHA" ] || fail "Waveshare header not restored for production"
fi

STORE_COUNT="$(grep -o '"panel_ui_store.c"' components/secure_bootstrap/CMakeLists.txt | wc -l | tr -d ' ')"
[ "$STORE_COUNT" = "1" ] || fail "panel_ui_store.c must appear once in CMake"
grep -F 'PANEL_UI_STORE_NAMESPACE "hpanel_ui"' components/secure_bootstrap/include/panel_ui_store.h >/dev/null || fail "namespace"
for KEY in '"cfg_a"' '"cfg_b"' '"active"'; do
  grep -F "$KEY" components/secure_bootstrap/include/panel_ui_store.h >/dev/null || fail "missing key $KEY"
done
for MARKER in '0x48505549' 'PANEL_UI_STORE_SCHEMA_VERSION' 'PANEL_UI_STORE_RECORD_SIZE' 'generation' 'crc32' 'nvs_commit' 'nvs_get_blob' 'nvs_set_blob' 'nvs_get_u8' 'nvs_set_u8'; do
  grep -R -F "$MARKER" components/secure_bootstrap/include/panel_ui_store.h components/secure_bootstrap/panel_ui_store.c >/dev/null || fail "missing store marker $MARKER"
done

grep -F 'nvs_get_blob(handle, slot_key(slot), NULL, &size)' components/secure_bootstrap/panel_ui_store.c >/dev/null || fail "missing NVS size query before blob read"
grep -F 'if (size != PANEL_UI_STORE_RECORD_SIZE)' components/secure_bootstrap/panel_ui_store.c >/dev/null || fail "missing wrong-size invalid-candidate handling"
if grep -R -n -E 'nvs_set_blob[^;]*(panel_ui_settings_t|&settings|sizeof\(settings\))' components/secure_bootstrap/panel_ui_store.c; then
  fail "raw settings struct blob detected"
fi

python3 components/secure_bootstrap/test_host/run_panel_ui_tests.py || fail "host tests"
git diff --check || fail "tracked git diff --check"

for FILE in $(git ls-files --others --exclude-standard); do
  git diff --no-index --check /dev/null "$FILE" >/dev/null 2>&1
  RC=$?
  [ "$RC" -le 1 ] || fail "untracked whitespace check: $FILE rc=$RC"
done
echo "Package 3A untracked no-index whitespace checks: PASS"

if grep -R -n -E 'HTTP_(PUT|POST|DELETE)|esp_http_client_set_method.*(PUT|POST|DELETE)|capability.*set|flow.*(run|execute)|advanced_flow.*(run|execute)|mood.*(run|execute)' components/secure_bootstrap/include/panel_ui_store.h components/secure_bootstrap/panel_ui_store.c; then
  fail "Package 2 store mutation scan"
else
  echo "Package 2 store mutation scan: PASS"
fi

if grep -R -n -E '(hpanel_wifi|hpanel_auth|access_token|refresh_token|client_secret|authorization|homey_id|device_id|capability|60bdcc6cfa595c0c05f97f9d)' components/secure_bootstrap/include/panel_ui_store.h components/secure_bootstrap/panel_ui_store.c; then
  fail "Package 2 store secrets scan"
else
  echo "Package 2 store secrets scan: PASS"
fi

[ -f components/secure_bootstrap/include/panel_ui.h ] || fail "missing panel_ui.h"
[ -f components/secure_bootstrap/panel_ui.c ] || fail "missing panel_ui.c"
[ "$(grep -o '"panel_ui.c"' components/secure_bootstrap/CMakeLists.txt | wc -l | tr -d ' ')" = "1" ] || fail "panel_ui.c count"
if grep -E 'lv_obj_t|lv_event_t|lv_style_t|#include[[:space:]]+"lvgl.h"' components/secure_bootstrap/include/panel_ui.h >/dev/null; then fail "panel_ui.h exposes LVGL"; fi
grep -F 'typedef struct panel_ui panel_ui_t;' components/secure_bootstrap/include/panel_ui.h >/dev/null || fail "opaque handle missing"
for M in lv_obj_set_scroll_snap_x LV_OBJ_FLAG_SCROLL_ONE LV_OBJ_FLAG_SNAPPABLE LV_EVENT_SCROLL_END lv_event_stop_processing create_read_only_card settings_layer confirmation_layer wake_overlay PANEL_BACKGROUND_BUILT_IN; do
  grep -F "$M" components/secure_bootstrap/panel_ui.c >/dev/null || fail "missing Package 3 marker $M"
done
if grep -E 'access_token|refresh_token|client_secret|authorization|homey_id|device_id|HTTP_(PUT|DELETE)|athom_auth_store_wipe|nvs_erase|lv_img|lv_image|fopen|filesystem|bsp_display_lock' components/secure_bootstrap/include/panel_ui.h components/secure_bootstrap/panel_ui.c >/dev/null; then fail "forbidden Package 3 marker"; fi
echo "Package 3 LVGL ownership scan: PASS"
echo "Package 3 mutation and secrets scan: PASS"

if grep -F 'bool panel_ui_tick(panel_ui_t' components/secure_bootstrap/include/panel_ui.h >/dev/null; then fail "panel_ui.h must not redeclare model symbol panel_ui_tick"; fi
if grep -F 'bool panel_ui_handle_touch(panel_ui_t' components/secure_bootstrap/include/panel_ui.h >/dev/null; then fail "panel_ui.h must not redeclare model symbol panel_ui_handle_touch"; fi
grep -F 'panel_ui_update_inactivity' components/secure_bootstrap/include/panel_ui.h >/dev/null || fail "missing panel_ui_update_inactivity"
grep -F 'panel_ui_process_touch' components/secure_bootstrap/include/panel_ui.h >/dev/null || fail "missing panel_ui_process_touch"
echo "Package 3 API collision regression scan: PASS"

grep -q '^CONFIG_BSP_DISPLAY_LVGL_BUF_HEIGHT=160$' sdkconfig.defaults || fail "sdkconfig.defaults LVGL buffer height must be 160"
python3 -c 'import ast, pathlib; ast.parse(pathlib.Path("tools/serial_monitor.py").read_text(encoding="utf-8"))' || fail "serial_monitor.py syntax"

if [ "$FAIL" -ne 0 ]; then
  echo "PATCH_012_PACKAGE3A_VALIDATION FAIL mode=$MODE"
  exit 1
fi

echo "PATCH_012_PACKAGE3A_VALIDATION PASS mode=$MODE"
exit 0
