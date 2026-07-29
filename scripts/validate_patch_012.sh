#!/bin/sh

BASE="7782ba329689490cbe5b78ba8be0298a6f046dc3"
BRANCH="patch-012-multi-page-dashboard-configuration-ui"
FAIL=0

fail() {
  echo "FAIL: $1"
  FAIL=1
}

changed_paths() {
  {
    git diff --name-only
    git diff --cached --name-only
    git ls-files --others --exclude-standard
  } | sed '/^$/d' | sort -u
}

[ "$(git branch --show-current)" = "$BRANCH" ] || fail "wrong branch"
[ "$(git rev-parse HEAD)" = "$BASE" ] || fail "unexpected HEAD"
[ "$(git merge-base HEAD "$BASE")" = "$BASE" ] || fail "wrong merge base"

CHANGED="$(changed_paths)"
COUNT="$(printf '%s\n' "$CHANGED" | sed '/^$/d' | wc -l | tr -d ' ')"
[ "$COUNT" = "17" ] || fail "expected 17 changed files, got $COUNT"

for FILE in $CHANGED; do
  case "$FILE" in
    components/secure_bootstrap/CMakeLists.txt|\
    components/secure_bootstrap/include/panel_ui.h|\
    components/secure_bootstrap/include/panel_ui_model.h|\
    components/secure_bootstrap/panel_ui_model.c|\
    components/secure_bootstrap/include/panel_ui_store.h|\
    components/secure_bootstrap/panel_ui.c|\
    components/secure_bootstrap/panel_ui_store.c|\
    components/secure_bootstrap/secure_bootstrap_esp.c|\
    components/secure_bootstrap/test_host/test_panel_ui_model.c|\
    components/secure_bootstrap/test_host/run_panel_ui_tests.py|\
    docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|\
    docs/handoff/MASTER_INDEX.md|\
    docs/handoff/CURRENT_STATE.md|\
    docs/handoff/HANDOFF.md|\
    docs/history/PATCH_HISTORY.md|\
    docs/history/PATCH_012_MULTI_PAGE_DASHBOARD_AND_CONFIGURATION_UI_FOUNDATION.md|\
    scripts/validate_patch_012.sh) ;;
    *) fail "unexpected changed file: $FILE" ;;
  esac
done

for FILE in \
  components/secure_bootstrap/include/secure_bootstrap.h \
  components/secure_bootstrap/include/phone_provisioning.h \
  components/secure_bootstrap/phone_provisioning_store.c
do
  printf '%s\n' "$CHANGED" | grep -Fxq "$FILE" && fail "forbidden changed file: $FILE"
done

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
echo "Package 2 untracked no-index whitespace checks: PASS"

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

# Package 3 LVGL app-shell guards
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


if [ "$FAIL" -ne 0 ]; then
  echo "PATCH_012_PACKAGE3_VALIDATION FAIL"
  exit 1
fi

echo "PATCH_012_PACKAGE3_VALIDATION PASS"
exit 0
