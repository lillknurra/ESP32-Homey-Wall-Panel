#!/bin/bash
BASE="11aea214f34162e7bb2012e046160bb7a4e1d59b"
BRANCH="patch-014-read-only-homey-dashboard-binding"

fail() {
    printf 'VALIDATE_PATCH_014 FAIL: %s\n' "$1"
    exit 1
}

[ "$(git branch --show-current)" = "$BRANCH" ] || fail "wrong branch"
[ "$(git rev-parse HEAD)" = "$BASE" ] || fail "HEAD changed before commit gate"
[ "$(git merge-base HEAD main)" = "$BASE" ] || fail "wrong merge base"
[ -z "$(git diff --cached --name-only)" ] || fail "index is not empty"

allowed_file="$(mktemp)"
changed_file="$(mktemp)"
trap 'rm -f "$allowed_file" "$changed_file"' EXIT

printf '%s\n' \
'components/secure_bootstrap/CMakeLists.txt' \
'components/secure_bootstrap/include/panel_homey_dashboard_binding.h' \
'components/secure_bootstrap/include/panel_ui_model.h' \
'components/secure_bootstrap/panel_homey_dashboard_binding.c' \
'components/secure_bootstrap/panel_ui.c' \
'components/secure_bootstrap/panel_ui_model.c' \
'components/secure_bootstrap/secure_bootstrap_esp.c' \
'components/secure_bootstrap/test_host/run_panel_ui_tests.py' \
'components/secure_bootstrap/test_host/test_panel_homey_dashboard_binding.c' \
'components/secure_bootstrap/test_host/test_panel_ui_model.c' \
'docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md' \
'docs/architecture/HOMEY_INVENTORY_CONTRACT.md' \
'docs/handoff/CURRENT_STATE.md' \
'docs/handoff/HANDOFF.md' \
'docs/handoff/MASTER_INDEX.md' \
'docs/history/PATCH_HISTORY.md' \
'docs/history/PATCH_014_READ_ONLY_HOMEY_SNAPSHOT_TO_DASHBOARD_BINDING_FOUNDATION.md' \
'scripts/validate_patch_014.sh' | sort > "$allowed_file"

git status --porcelain=v1 --untracked-files=all | cut -c4- | sed 's/.* -> //' | sort -u > "$changed_file"
comm -23 "$changed_file" "$allowed_file" | grep -q . && fail "path outside approved scope"

for forbidden in \
'components/secure_bootstrap/athom_cloud_client.c' \
'components/secure_bootstrap/include/athom_cloud_client.h' \
'components/secure_bootstrap/panel_homey_read_snapshot.c' \
'components/secure_bootstrap/include/panel_homey_read_snapshot.h' \
'components/secure_bootstrap/include/panel_homey_alias_provider.h' \
'components/secure_bootstrap/include/secure_bootstrap.h' \
'components/secure_bootstrap/include/phone_provisioning.h' \
'components/secure_bootstrap/phone_provisioning_store.c' \
'main/main.c' \
'sdkconfig.defaults' \
'dependencies.lock' \
'tools/serial_monitor.py'
do
    git diff --quiet "$BASE" -- "$forbidden" || fail "forbidden file changed: $forbidden"
done

git diff --check || fail "git diff --check"
python3 components/secure_bootstrap/test_host/run_panel_ui_tests.py || fail "host tests"

grep -q 'HOMEY_DASHBOARD_POLL_INTERVAL_MS 1000' components/secure_bootstrap/secure_bootstrap_esp.c || fail "1000 ms poll missing"
grep -q 'athom_cloud_copy_device_snapshot' components/secure_bootstrap/secure_bootstrap_esp.c || fail "snapshot copy missing"
grep -q 'panel_homey_dashboard_apply_snapshot' components/secure_bootstrap/secure_bootstrap_esp.c || fail "adapter call missing"
grep -q 'panel_ui_apply_homey_dashboard_state' components/secure_bootstrap/secure_bootstrap_esp.c || fail "model apply missing"

git diff "$BASE" -- components/secure_bootstrap | grep '^+' | grep -E 'HTTP_(POST|PUT|PATCH|DELETE)|esp_http_client_set_method|request_.*(flow|mood|device|capability)' && fail "mutation-like addition detected"

git diff "$BASE" -- components/secure_bootstrap | grep '^+' | grep -E 'access_token|refresh_token|authorization_code|Bearer [A-Za-z0-9]' && fail "secret-like addition detected"

grep -A 12 '### Patch 013 accepted post-merge state' docs/handoff/CURRENT_STATE.md | grep -q 'COMPLETE' || fail "Patch 013 complete state missing"
grep -q 'RUNTIME NOT RUN' docs/handoff/CURRENT_STATE.md || fail "Patch 013 runtime state missing"
grep -q 'NON_BLOCKING_TECHNICAL_NOTE' docs/handoff/CURRENT_STATE.md || fail "volatile note missing"
grep -q 'Package 3B.*NOT_STARTED' docs/handoff/CURRENT_STATE.md || fail "Package 3B state missing"
grep -A 24 '## Patch 014 verified build and runtime evidence' docs/handoff/CURRENT_STATE.md | grep -q 'Normal flash: `PASS`' || fail "Patch 014 flash evidence missing"
grep -A 24 '## Patch 014 verified build and runtime evidence' docs/handoff/CURRENT_STATE.md | grep -q 'Bounded serial runtime: `PASS`' || fail "Patch 014 runtime evidence missing"
grep -q 'IRAM: `16,384 / 16,384 bytes`' docs/history/PATCH_014_READ_ONLY_HOMEY_SNAPSHOT_TO_DASHBOARD_BINDING_FOUNDATION.md || fail "Patch 014 IRAM evidence missing"
grep -q '11aea214f34162e7bb2012e046160bb7a4e1d59b' docs/handoff/CURRENT_STATE.md || fail "stable baseline missing"

printf 'VALIDATE_PATCH_014 PASS\n'
