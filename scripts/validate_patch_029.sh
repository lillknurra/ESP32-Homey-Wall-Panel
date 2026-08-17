#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH029_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-029-repair-homey-favorites-status-binding" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "eb4fb252d09482e65da2779eb10cf039bf971bc6" ] || fail "base_head"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

allowed='components/secure_bootstrap/athom_cloud_client.c
components/secure_bootstrap/include/athom_cloud_client.h
components/secure_bootstrap/athom_oauth_runtime.c
components/secure_bootstrap/panel_homey_favorites.c
components/secure_bootstrap/include/panel_homey_favorites.h
components/secure_bootstrap/test_host/test_panel_homey_favorites.c
docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_029_HOMEY_FAVORITES_STATUS_BINDING.md
scripts/validate_patch_029.sh'
actual="$(git status --short | sed -E 's/^.. //')"
while IFS= read -r path; do
    [ -z "$path" ] && continue
    printf '%s\n' "$allowed" | grep -Fx "$path" >/dev/null || fail "scope_$path"
done <<EOF
$actual
EOF

HEADER="components/secure_bootstrap/include/panel_homey_favorites.h"
FAVORITES="components/secure_bootstrap/panel_homey_favorites.c"
CLOUD="components/secure_bootstrap/athom_cloud_client.c"
OAUTH="components/secure_bootstrap/athom_oauth_runtime.c"
TEST="components/secure_bootstrap/test_host/test_panel_homey_favorites.c"

grep -F 'PANEL_HOMEY_FAVORITES_VALID_CONFIGURED' "$HEADER" >/dev/null || fail "configured_state_missing"
grep -F 'PANEL_HOMEY_FAVORITES_VALID_EMPTY' "$HEADER" >/dev/null || fail "empty_state_missing"
grep -F 'PANEL_HOMEY_FAVORITES_UNVERIFIED' "$HEADER" >/dev/null || fail "unverified_state_missing"
grep -F 'panel_homey_favorites_get_state' "$FAVORITES" >/dev/null || fail "state_accessor_missing"
grep -F 'panel_homey_favorites_state_name' "$FAVORITES" >/dev/null || fail "state_name_missing"
grep -F 'panel_homey_favorites_parse_and_publish' "$CLOUD" >/dev/null || fail "favorites_publish_missing"
grep -F 'panel_homey_snapshot_publish_json' "$CLOUD" >/dev/null || fail "snapshot_publish_missing"
grep -F '"/api/manager/zones/zone"' "$CLOUD" >/dev/null || fail "zones_endpoint_missing"
grep -F '"/api/manager/devices/device"' "$CLOUD" >/dev/null || fail "devices_endpoint_missing"
grep -F 'favorites_state=%s' "$OAUTH" >/dev/null || fail "readiness_state_missing"
grep -F 'verified_favorites=true' "$OAUTH" >/dev/null && fail "hardcoded_favorites_verification"
grep -F 'PANEL_WIDGET_UNKNOWN' "$FAVORITES" >/dev/null || fail "unknown_binding_missing"
grep -F 'PANEL_WIDGET_UNCONFIGURED' "$FAVORITES" >/dev/null || fail "empty_binding_missing"
grep -F 'test_valid_empty_is_unconfigured' "$TEST" >/dev/null || fail "empty_test_missing"
grep -F 'test_unverified_matching_failure_is_unknown' "$TEST" >/dev/null || fail "matching_test_missing"
grep -F 'test_capability_failure_is_unverified' "$TEST" >/dev/null || fail "capability_test_missing"

changed_source="$(git diff -- components/secure_bootstrap/athom_cloud_client.c components/secure_bootstrap/athom_oauth_runtime.c components/secure_bootstrap/panel_homey_favorites.c components/secure_bootstrap/include/panel_homey_favorites.h components/secure_bootstrap/test_host/test_panel_homey_favorites.c)"
printf '%s\n' "$changed_source" | grep -Eiq 'access_token|refresh_token|Authorization|Bearer|https?://|ssid|homey_id|device_id|capability_id|response_body|headers' && fail "source_privacy_or_policy_marker"

git diff --check || fail "diff_check"
printf 'PATCH029_STATIC=PASS\n'
printf 'PATCH029_FAVORITES_STATE=VALID_CONFIGURED_VALID_EMPTY_UNVERIFIED\n'
printf 'PATCH029_TRANSPORT_POLICY=UNCHANGED\n'
printf 'PATCH029_RUNTIME=NOT_RUN\n'
