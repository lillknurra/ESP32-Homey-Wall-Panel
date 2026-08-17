#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH028_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-028-homey-startup-status-readiness-optimization" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "717d025e071df75551fc203fc96b7d2e79307aa8" ] || fail "base_head"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

allowed='components/secure_bootstrap/secure_bootstrap_esp.c
components/secure_bootstrap/panel_ui.c
components/secure_bootstrap/include/panel_ui.h
components/secure_bootstrap/test_host/test_panel_ui_model.c
docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_028_HOMEY_STARTUP_STATUS_READINESS_OPTIMIZATION.md
scripts/validate_patch_028.sh'
actual="$(git status --short | sed -E 's/^.. //')"
while IFS= read -r path; do
    [ -z "$path" ] && continue
    printf '%s\n' "$allowed" | grep -Fx "$path" >/dev/null || fail "scope_$path"
done <<EOF
$actual
EOF

ESP="components/secure_bootstrap/secure_bootstrap_esp.c"
UI="components/secure_bootstrap/panel_ui.c"
HEADER="components/secure_bootstrap/include/panel_ui.h"

grep -F 'HOMEY_DATA_UI phase=dashboard_shell verified=false privacy=sanitized' "$ESP" >/dev/null || fail "shell_marker"
grep -F 'panel_ui_set_homey_data_ready(s_panel_ui, false)' "$ESP" >/dev/null || fail "shell_not_neutral"
grep -F 'panel_ui_set_homey_data_ready(s_panel_ui, true)' "$ESP" >/dev/null || fail "ready_transition_missing"
grep -F 'ATHOM_HOMEY_DATA_READY' "$ESP" >/dev/null || fail "ready_authority_missing"
grep -F 'bool panel_ui_set_homey_data_ready(panel_ui_t *ui, bool ready);' "$HEADER" >/dev/null || fail "readiness_api_missing"
grep -F 'ui->homey_data_ready' "$UI" >/dev/null || fail "ui_readiness_gate_missing"
grep -F 'LV_STATE_DISABLED' "$UI" >/dev/null || fail "homey_controls_not_disabled"
grep -F 'const bool favorites_changed = ui->homey_data_ready' "$UI" >/dev/null || fail "favorites_pre_ready_guard_missing"
grep -F 'if (athom_oauth_runtime_homey_data_state() != ATHOM_HOMEY_DATA_READY)' "$ESP" >/dev/null || fail "poll_ready_guard_missing"

if git diff --name-only | grep -E '(^|/)(athom_cloud_client\.c|athom_oauth_runtime\.c|sdkconfig|managed_components/)' >/dev/null; then
    fail "forbidden_transport_or_config_scope"
fi

if git diff -- components/secure_bootstrap/secure_bootstrap_esp.c components/secure_bootstrap/panel_ui.c components/secure_bootstrap/include/panel_ui.h | grep -E 'PATCH022_SCROLL_(THROW|LIMIT)|lv_indev_set_scroll_(throw|limit)|/api/manager|access_token|refresh_token|Authorization|Bearer' >/dev/null; then
    fail "forbidden_policy_or_secret_marker"
fi

grep -F 'PATCH028' docs/handoff/CURRENT_STATE.md >/dev/null || fail "state_marker_missing"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
git diff --check || fail "diff_check"

printf 'PATCH028_STATIC=PASS\n'
printf 'PATCH028_SCOPE=BOUNDED_PRE_READY_UI_SHELL\n'
printf 'PATCH028_TRANSPORT_POLICY=UNCHANGED\n'
printf 'PATCH028_RUNTIME=NOT_RUN\n'
