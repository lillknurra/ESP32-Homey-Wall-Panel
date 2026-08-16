#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH025_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-025-bound-homey-inventory-schema-diagnostics" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "335694989ed68bc0285be4d0ea5e64982f2b8a73" ] || fail "base_head"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

allowed='components/secure_bootstrap/athom_cloud_client.c
docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS.md
scripts/validate_patch_025.sh'
actual="$(git status --short | sed -E 's/^.. //')"
if [ "$(printf '%s\n' "$actual" | sort)" != "$(printf '%s\n' "$allowed" | sort)" ]; then
    fail "exact_scope"
fi

CLIENT="components/secure_bootstrap/athom_cloud_client.c"
grep -F 'static const bool detail_logging_enabled = false;' "$CLIENT" >/dev/null || fail "detail_gate"
grep -F 'HOMEY_SCHEMA summary=bounded detail_logging=false privacy=sanitized' "$CLIENT" >/dev/null || fail "bounded_summary"
grep -F 'homey_schema_log_inventory(response);' "$CLIENT" >/dev/null || fail "schema_call_missing"
grep -F 'panel_homey_snapshot_publish_json(' "$CLIENT" >/dev/null || fail "snapshot_publish_missing"
grep -F 'panel_homey_favorites_parse_and_publish(' "$CLIENT" >/dev/null || fail "favorites_publish_missing"
grep -F '"/api/manager/zones/zone"' "$CLIENT" >/dev/null || fail "zones_endpoint_missing"
grep -F '"/api/manager/devices/device"' "$CLIENT" >/dev/null || fail "devices_endpoint_missing"
grep -F '#define CLOUD_HTTP_TIMEOUT_MS 8000' "$CLIENT" >/dev/null || fail "cloud_timeout_changed"
grep -F '#define HOMEY_REMOTE_HTTP_TIMEOUT_MS 8000' "$CLIENT" >/dev/null || fail "homey_timeout_changed"

summary_line="$(grep -F 'HOMEY_SCHEMA summary=bounded' "$CLIENT")"
printf '%s\n' "$summary_line" | grep -Eiq 'token|homey_id|device_id|capability_id|https?://|ssid|response|header|bearer' && fail "summary_privacy"

grep -F 'Patch025 - Bounded Homey Inventory Schema Diagnostics' docs/history/PATCH_025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS.md >/dev/null || fail "history_title"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F 'PATCH025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_active_patch"

git diff --check || fail "diff_check"
printf 'PATCH025_STATIC=PASS\n'
printf 'PATCH025_DETAIL_LOGGING=BOUNDED_ONE_SUMMARY\n'
printf 'PATCH025_MUTATION=PROHIBITED\n'
printf 'PATCH025_RUNTIME=NOT_RUN\n'
