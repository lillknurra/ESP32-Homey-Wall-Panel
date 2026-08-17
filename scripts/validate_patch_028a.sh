#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH028A_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-028a-neutralize-pre-ready-homey-status-shell" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "8bb4ddfc1ed7f78d1523ea359fdf0c07835674bb" ] || fail "base_head"
[ -z "$(git diff --cached --name-only)" ] || fail "staged_changes"

allowed='components/secure_bootstrap/panel_ui.c
components/secure_bootstrap/test_host/test_panel_ui_model.c
docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_028A_PRE_READY_HOMEY_STATUS_SHELL.md
scripts/validate_patch_028a.sh'

actual="$(git status --short | sed -E 's/^.. //')"
while IFS= read -r path; do
    [ -z "$path" ] && continue
    printf '%s\n' "$allowed" | grep -Fx "$path" >/dev/null || fail "scope_$path"
done <<EOF
$actual
EOF

grep -F 'PATCH028A: keep the model' components/secure_bootstrap/panel_ui.c >/dev/null || fail "guard_marker"
grep -F 'if (!ui->homey_data_ready)' components/secure_bootstrap/panel_ui.c >/dev/null || fail "readiness_guard"
[ "$(grep -c 'panel_ui_render_status_text(' components/secure_bootstrap/panel_ui.c)" -eq 4 ] || fail "render_path_coverage"
grep -F 'PATCH_028A=ACTIVE_BOUNDED_PRE_READY_STATUS_SHELL' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_state"
grep -F '## Patch028A - Neutralize Pre-Ready Homey Status Shell' docs/history/PATCH_HISTORY.md >/dev/null || fail "history"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime"

firmware_diff="$(git diff --unified=0 -- components/secure_bootstrap/panel_ui.c)"
changed_lines="$(printf '%s\n' "$firmware_diff" | sed -n '/^+/p' | grep -v '^+++' || true)"
if printf '%s\n' "$changed_lines" | grep -E 'athom_cloud|oauth|retry|timeout|reconnect|scroll_throw|scroll_limit|mutation|command dispatch|lv_obj_set_size|lv_obj_scroll_to' >/dev/null; then
    fail "forbidden_firmware_change"
fi

git diff --check || fail "diff_check"

printf 'PATCH028A_STATIC=PASS\n'
printf 'PATCH028A_SCOPE=BOUNDED_PRE_READY_HOMEY_STATUS_SHELL\n'
printf 'PATCH028A_RUNTIME=NOT_RUN\n'
