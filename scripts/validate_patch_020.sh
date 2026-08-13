#!/bin/sh

fail() {
  printf 'PATCH020_VALIDATION=FAIL %s\n' "$1" >&2
  exit 1
}

current_branch=$(git branch --show-current) || fail "branch_check"
test "$current_branch" = "patch-020-post-patch019a17-repository-reconciliation" || fail "wrong_branch"

head_sha=$(git rev-parse HEAD) || fail "head_check"
test "$head_sha" = "482064da7620accda2bc6768ad6b847ebd7bf473" || fail "wrong_head"

staged=$(git diff --cached --name-only) || fail "staged_check"

allowed='^(docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md|docs/architecture/ATHOM_OAUTH_AND_HOMEY_SELECTION_UX.md|docs/architecture/HOMEY_INVENTORY_CONTRACT.md|docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md|docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md|scripts/validate_patch_020.sh)$'

staged_unexpected=$(printf '%s\n' "$staged" | grep -v '^$' | grep -Ev "$allowed" || true)
test -z "$staged_unexpected" || fail "unexpected_staged_files"

tracked_changed=$(git diff --name-only -- .) || fail "changed_files"
staged_changed=$(git diff --cached --name-only -- .) || fail "staged_changed_files"
untracked_changed=$(git ls-files --others --exclude-standard) || fail "untracked_files"
changed=$(printf '%s\n%s\n%s\n' "$tracked_changed" "$staged_changed" "$untracked_changed")
allowed_non_patch018=$(printf '%s\n' "$changed" | grep -v '^$' | grep -v '^components/secure_bootstrap/panel_ui.c$' || true)
unexpected=$(printf '%s\n' "$allowed_non_patch018" | grep -Ev "$allowed" || true)
test -z "$unexpected" || fail "unexpected_files"

components_changed=$(printf '%s\n' "$allowed_non_patch018" | grep '^components/' || true)
test -z "$components_changed" || fail "components_scope_changed"

git diff --quiet -- components/secure_bootstrap/panel_ui.c || panel_ui_dirty=yes
test "${panel_ui_dirty:-no}" = "yes" || fail "expected_patch018_panel_ui_dirty_not_observed"

grep -F 'STABLE_IMPLEMENTATION_MERGE=482064da7620accda2bc6768ad6b847ebd7bf473' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_merge_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH_020_POST_PATCH019A17_REPOSITORY_RECONCILIATION' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch_missing"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime_boundary_missing"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_boundary_missing"
grep -F 'PATCH_018=PAUSED_LOCAL_DIRTY_PANEL_UI_ONLY' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch018_boundary_missing"

grep -F '482064da7620accda2bc6768ad6b847ebd7bf473' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_stable_merge_missing"
grep -F 'docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "patch017_index_missing"
grep -F 'docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "patch019_index_missing"

grep -F '253319f361b9967ebcaca376591bd14ecf3d9c0e' docs/history/PATCH_017_VERIFIED_HOMEY_FAVORITES_BINDING.md >/dev/null || fail "patch017_sha_missing"
grep -F '482064da7620accda2bc6768ad6b847ebd7bf473' docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md >/dev/null || fail "patch019_sha_missing"
grep -F 'PATCH019A17V2_RUNTIME_ACCEPTANCE=PASS' docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md >/dev/null || fail "patch019_runtime_pass_missing"
grep -F 'NOT_OBSERVED' docs/history/PATCH_019A17_CLOUD_TO_HOMEY_TLS_LIFECYCLE_HANDOFF.md >/dev/null || fail "patch019_not_observed_missing"

if git diff -- docs/handoff docs/history docs/architecture scripts | grep -E '(access_token|refresh_token|Authorization: Bearer|client_secret)[[:space:]]*[:=][[:space:]]*[^`[:space:]]' >/dev/null; then
  fail "secret_pattern"
fi

if grep -R -n 'ACTIVE_DEVELOPMENT_PATCH=PATCH_016_LIVE_READ_ONLY_FAVORITE_LIGHT_STATUS' docs/handoff docs/history/PATCH_HISTORY.md >/dev/null; then
  fail "stale_patch016_active_status"
fi

printf 'PATCH020_VALIDATION=PASS\n'
