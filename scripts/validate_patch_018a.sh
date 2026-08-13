#!/bin/sh

fail() {
  printf 'PATCH018A_VALIDATION=FAIL %s\n' "$1" >&2
  exit 1
}

current_branch=$(git branch --show-current) || fail "branch_check"
test "$current_branch" = "patch-018a-panel-ui-swipe-diagnostic-resolution" || fail "wrong_branch"

head_sha=$(git rev-parse HEAD) || fail "head_check"
test "$head_sha" = "c6642b081b35e823853d973dd3127c5ce3dabbad" || fail "wrong_head"

main_sha=$(git rev-parse main) || fail "main_check"
origin_main_sha=$(git rev-parse origin/main) || fail "origin_main_check"
test "$main_sha" = "c6642b081b35e823853d973dd3127c5ce3dabbad" || fail "wrong_main"
test "$origin_main_sha" = "c6642b081b35e823853d973dd3127c5ce3dabbad" || fail "wrong_origin_main"

staged=$(git diff --cached --name-only) || fail "staged_check"
test -z "$staged" || fail "staged_files_present"

allowed='^(components/secure_bootstrap/panel_ui.c|docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md|scripts/validate_patch_018a.sh)$'

tracked_changed=$(git diff --name-only -- .) || fail "changed_files"
untracked_changed=$(git ls-files --others --exclude-standard) || fail "untracked_files"
changed=$(printf '%s\n%s\n' "$tracked_changed" "$untracked_changed")
unexpected=$(printf '%s\n' "$changed" | grep -v '^$' | grep -Ev "$allowed" || true)
test -z "$unexpected" || fail "unexpected_files"

panel_components_changed=$(printf '%s\n' "$changed" | grep '^components/' | grep -v '^components/secure_bootstrap/panel_ui.c$' || true)
test -z "$panel_components_changed" || fail "unexpected_components_scope"

test -f docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md || fail "history_file_missing"
test -f scripts/validate_patch_018a.sh || fail "validator_file_missing"

grep -F 'PATCH018_SWIPE_BEGIN' components/secure_bootstrap/panel_ui.c >/dev/null || fail "begin_marker_missing"
grep -F 'PATCH018_SWIPE_END' components/secure_bootstrap/panel_ui.c >/dev/null || fail "end_marker_missing"
grep -F 'patch018_swipe_gesture_event' components/secure_bootstrap/panel_ui.c >/dev/null || fail "gesture_handler_missing"
grep -F 'LV_EVENT_SCROLL_BEGIN' components/secure_bootstrap/panel_ui.c >/dev/null || fail "scroll_begin_missing"
grep -F 'LV_EVENT_SCROLL_END' components/secure_bootstrap/panel_ui.c >/dev/null || fail "scroll_end_missing"
grep -F 'esp_log.h' components/secure_bootstrap/panel_ui.c >/dev/null || fail "esp_log_include_missing"

grep -F 'STABLE_REPOSITORY_MERGE=c6642b081b35e823853d973dd3127c5ce3dabbad' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_repository_merge_missing"
grep -F 'STABLE_IMPLEMENTATION_MERGE=482064da7620accda2bc6768ad6b847ebd7bf473' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_implementation_merge_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch_missing"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=patch-018a-panel-ui-swipe-diagnostic-resolution' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_branch_missing"
grep -F 'PATCH_020=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch020_complete_missing"
grep -F 'PATCH_020_REMOTE_BRANCH_CLEANUP=COMPLETE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch020_cleanup_missing"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime_boundary_missing"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_boundary_missing"

grep -F 'docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_history_missing"
grep -F 'Patch018A - resolve paused panel UI swipe diagnostics' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_active_patch_missing"
grep -F 'Patch020: `COMPLETE / MERGED`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_patch020_complete_missing"
grep -F 'Patch018A is the only active development patch' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_active_patch_missing"
grep -F 'Patch020 - Post-Patch019A1.7 Repository Reconciliation' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch020_history_missing"
grep -F 'Squash merge commit:' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch020_merge_label_missing"
grep -F 'c6642b081b35e823853d973dd3127c5ce3dabbad' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch020_merge_missing"
grep -F 'Patch018A - Panel UI Swipe Diagnostic Resolution' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018a_history_missing"
grep -F 'Remote cleanup' docs/history/PATCH_HISTORY.md >/dev/null || fail "remote_cleanup_history_missing"

if git diff -- docs/handoff docs/history scripts components/secure_bootstrap/panel_ui.c | grep -E '(access_token|refresh_token|Authorization: Bearer|client_secret)[[:space:]]*[:=][[:space:]]*[^`[:space:]]' >/dev/null; then
  fail "secret_pattern"
fi

if git diff -- docs/handoff docs/history scripts components/secure_bootstrap/panel_ui.c | grep -E '(homey.device.control|setCapabilityValue|triggerFlow|runFlow|activateMood)' >/dev/null; then
  fail "mutation_or_scope_pattern"
fi

printf 'PATCH018A_VALIDATION=PASS\n'
