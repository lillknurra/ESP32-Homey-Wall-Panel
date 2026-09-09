#!/bin/sh
set -eu

BASE="5a4075e045b8af77394934ef7ec19068e480d615"
BRANCH="patch-036-verified-homey-favorite-light-toggle-execution-readiness-gate"
H="components/secure_bootstrap/include/panel_homey_favorites.h"
C="components/secure_bootstrap/panel_homey_favorites.c"
T="components/secure_bootstrap/test_host/test_panel_homey_favorites.c"
V="scripts/validate_patch_036.sh"

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

fail() {
    printf '%s\n' "PATCH036_STATIC_VALIDATOR=FAIL" >&2
    printf '%s\n' "PATCH036_STATIC_VALIDATOR_REASON=$1" >&2
    exit 1
}

[ "$(git branch --show-current)" = "$BRANCH" ] || fail "wrong_branch"
[ "$(git rev-parse HEAD)" = "$BASE" ] || fail "head_not_stable_base"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes_present"

STATUS="$(git status --porcelain=v1 --untracked-files=all)"
COUNT="$(printf '%s\n' "$STATUS" | sed '/^$/d' | wc -l | tr -d ' ')"
[ "$COUNT" = "4" ] || fail "unexpected_worktree_path_count"
printf '%s\n' "$STATUS" | grep -Fqx " M $H" || fail "header_status"
printf '%s\n' "$STATUS" | grep -Fqx " M $C" || fail "source_status"
printf '%s\n' "$STATUS" | grep -Fqx " M $T" || fail "test_status"
printf '%s\n' "$STATUS" | grep -Fqx "?? $V" || fail "validator_status"

[ "$(grep -Fc 'bool panel_homey_favorites_light_toggle_execution_ready(' "$H")" = "1" ] || fail "public_api_count"
[ "$(grep -Fc 'bool panel_homey_favorites_light_toggle_execution_ready(' "$C")" = "1" ] || fail "implementation_count"
grep -Fq 'bool homey_data_ready);' "$H" || fail "homey_data_ready_api"
grep -Fq 'if (!homey_data_ready || widget_index < 4U ||' "$C" || fail "homey_data_ready_guard"
grep -Fq 'widget_index >= 4U + PANEL_HOMEY_FAVORITE_LIMIT)' "$C" || fail "widget_range_guard"
grep -Fq 's_public.state == PANEL_HOMEY_FAVORITES_VALID_CONFIGURED' "$C" || fail "favorites_state_guard"
grep -Fq 'slot < s_public.count' "$C" || fail "slot_exists_guard"
grep -Fq 'item->available &&' "$C" || fail "availability_guard"
grep -Fq 'item->onoff_known &&' "$C" || fail "onoff_known_guard"
grep -Fq 'item->onoff_command_eligible &&' "$C" || fail "eligibility_guard"
grep -Fq 'item->light_toggle_authorized;' "$C" || fail "authorization_guard"
grep -Fq 'FAVORITES_LOCK();' "$C" || fail "favorites_lock_missing"
grep -Fq 'FAVORITES_UNLOCK();' "$C" || fail "favorites_unlock_missing"

for name in \
    test_light_toggle_execution_readiness_valid_widget4_widget5 \
    test_light_toggle_execution_readiness_requires_current_homey_data_ready \
    test_light_toggle_execution_readiness_state_slot_and_widget_guards \
    test_light_toggle_execution_readiness_requires_item_authorities
do
    grep -Fq "$name" "$T" || fail "missing_test_$name"
done
grep -Fq 'PATCH036_HOMEY_LIGHT_TOGGLE_EXECUTION_READINESS_TEST PASS' "$T" || fail "missing_test_pass_marker"

PROD_ADDED="$(git diff --no-ext-diff --no-color "$BASE" -- "$H" "$C" | sed -n '/^+++/d; /^+/s/^+//p')"
if printf '%s\n' "$PROD_ADDED" | grep -Eiq 'setCapabilityValue|homey\.device\.control|HTTP_METHOD_(PUT|POST|DELETE)|esp_http_client|/api/manager/|nvs_(set|erase|commit)|panel_homey_alias_store_(publish|wipe)|lv_obj|LV_OBJ_FLAG_CLICKABLE|ESP_LOG|printf\(|snprintf\(|raw_[a-z_]*id|device_id|homey_id'; then
    fail "forbidden_production_surface_added"
fi

TRACKED="$(git diff --name-only "$BASE")"
[ "$(printf '%s\n' "$TRACKED" | sed '/^$/d' | wc -l | tr -d ' ')" = "3" ] || fail "tracked_scope_count"
printf '%s\n' "$TRACKED" | grep -Fqx "$H" || fail "tracked_scope_header"
printf '%s\n' "$TRACKED" | grep -Fqx "$C" || fail "tracked_scope_source"
printf '%s\n' "$TRACKED" | grep -Fqx "$T" || fail "tracked_scope_test"

[ -f "$V" ] || fail "validator_missing"
[ ! -e "docs/history/PATCH_036_HOMEY_FAVORITE_LIGHT_TOGGLE_EXECUTION_READINESS_GATE.md" ] || fail "documentation_added"

git diff --check "$BASE" -- "$H" "$C" "$T" >/dev/null || fail "tracked_diff_check"

printf '%s\n' "PATCH036_STATIC_VALIDATOR=PASS"
printf '%s\n' "PATCH036_HOMEY_MUTATION=NO_BY_SCOPE_AND_ADDED_SOURCE_SCAN"
printf '%s\n' "PATCH036_COMMAND_DISPATCH=NO_BY_SCOPE_AND_ADDED_SOURCE_SCAN"
printf '%s\n' "PATCH036_UI_CLICKABILITY_CHANGE=NO_BY_EXACT_SCOPE"
printf '%s\n' "PATCH036_DOCUMENTATION_RECONCILIATION=NO_BY_EXACT_SCOPE"
