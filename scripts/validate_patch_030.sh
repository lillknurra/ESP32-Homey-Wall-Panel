#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH030_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-030-bounded-panel-ui-render-path-stabilization" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "8d70f26262ea71f280a235c009ba6f7c12461cee" ] || fail "base_head"
[ -z "$(git diff --cached --name-only)" ] || fail "staged_changes"

allowed='components/secure_bootstrap/panel_ui.c
docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_030_PANEL_UI_RENDER_PATH_STABILIZATION.md
scripts/validate_patch_030.sh'

actual="$(git status --short | sed -E 's/^.. //')"
while IFS= read -r path; do
    [ -z "$path" ] && continue
    printf '%s\n' "$allowed" | grep -Fx "$path" >/dev/null || fail "scope_$path"
done <<EOF
$actual
EOF

grep -F 'const bool page_reassert = !pager_scrolling_start;' components/secure_bootstrap/panel_ui.c >/dev/null || fail "guard_missing"
grep -F 'page_reassert=%s' components/secure_bootstrap/panel_ui.c >/dev/null || fail "diagnostic_marker"
grep -F '## Patch 030 - Bounded Panel UI Render-Path Stabilization' docs/history/PATCH_HISTORY.md >/dev/null || fail "history"
grep -F 'PATCH_030=ACTIVE_BOUNDED_FIRMWARE_IMPLEMENTATION' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_state"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime"

firmware_diff="$(git diff --unified=0 -- components/secure_bootstrap/panel_ui.c)"
changed_lines="$(printf '%s\n' "$firmware_diff" | sed -n '/^+/p' | grep -v '^+++' || true)"
if printf '%s\n' "$changed_lines" | grep -E 'scroll_throw|scroll_limit|athom_cloud|oauth|retry|timeout|reconnect|mutation|command dispatch' >/dev/null; then
    fail "forbidden_firmware_change"
fi

git diff --check || fail "diff_check"

printf 'PATCH030_STATIC=PASS\n'
printf 'PATCH030_SCOPE=BOUNDED_PANEL_UI_RENDER_PATH\n'
printf 'PATCH030_RUNTIME=NOT_RUN\n'
