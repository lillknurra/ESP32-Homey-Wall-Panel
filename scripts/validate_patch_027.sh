#!/usr/bin/env bash
set -u

fail() {
    printf 'PATCH027_STATIC_VALIDATION=FAIL reason=%s\n' "$1" >&2
    exit 1
}

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$repo_dir" || exit 1

is_allowed_scope_path() {
    case "$1" in
        docs/handoff/MASTER_INDEX.md|\
        docs/handoff/CURRENT_STATE.md|\
        docs/handoff/HANDOFF.md|\
        docs/history/PATCH_HISTORY.md|\
        docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md|\
        scripts/validate_patch_027.sh)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

actual_scope=$( { git diff --name-only HEAD; git ls-files --others --exclude-standard; } ) || fail "scope_read"
for path in $actual_scope; do
    is_allowed_scope_path "$path" || fail "scope:$path"
done

for required in \
    docs/handoff/MASTER_INDEX.md \
    docs/handoff/CURRENT_STATE.md \
    docs/handoff/HANDOFF.md \
    docs/history/PATCH_HISTORY.md \
    docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md \
    scripts/validate_patch_027.sh; do
    printf '%s\n' "$actual_scope" | grep -Fx "$required" >/dev/null || fail "scope_missing:$required"
done

[ "$(git branch --show-current)" = "patch-027-select-first-package-3b-command-slice" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "021f547d6397e4ff42b7c8505af02a1bd72108af" ] || fail "base_head"
[ "$(git rev-parse main)" = "021f547d6397e4ff42b7c8505af02a1bd72108af" ] || fail "local_main"
[ "$(git rev-parse origin/main)" = "021f547d6397e4ff42b7c8505af02a1bd72108af" ] || fail "origin_main"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

grep -F 'STABLE_REPOSITORY_MERGE=021f547d6397e4ff42b7c8505af02a1bd72108af' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_merge"
grep -F 'STABLE_IMPLEMENTATION_MERGE=021f547d6397e4ff42b7c8505af02a1bd72108af' docs/handoff/CURRENT_STATE.md >/dev/null || fail "implementation_merge"
grep -F 'PATCH_026=COMPLETE_MERGED_REQUIREMENTS_SCOPE_LOCK' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch026_state"
grep -F 'PATCH_026_PR=37' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch026_pr"
grep -F 'PATCH_026_MERGE=021f547d6397e4ff42b7c8505af02a1bd72108af' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch026_merge"
grep -F 'PATCH_027=ACTIVE_COMMAND_SLICE_SCOPE_LOCK' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch027_state"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=patch-027-select-first-package-3b-command-slice' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch027_branch"
grep -F 'PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "command_selection_boundary"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F 'Patch027 is active on branch' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_patch027"
grep -F 'Patch027 - Select First Package 3B Command Slice' docs/history/PATCH_HISTORY.md >/dev/null || fail "history_section"
grep -F 'The first user-visible Package 3B command remains `NOT_SELECTED`.' docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md >/dev/null || fail "not_selected_boundary"
grep -F 'ATHOM_HOMEY_COMMAND_REFRESH_INVENTORY_SCHEMA' docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md >/dev/null || fail "refresh_boundary"
grep -F 'No exact' docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md >/dev/null || fail "no_guessing_boundary"

component_status=$(git status --porcelain -- components managed_components)
[ -z "$component_status" ] || fail "firmware_or_managed_component_changes"

git diff --check >/dev/null 2>&1 || fail "diff_check"

printf 'PATCH027_STATIC_VALIDATION=PASS\n'
printf 'PATCH027_SCOPE=DOCUMENTATION_ONLY\n'
printf 'PACKAGE_3B=NOT_STARTED\n'
printf 'PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED\n'
printf 'PATCH027_RUNTIME=NOT_APPLICABLE\n'
