#!/usr/bin/env bash
set -u

fail() {
    printf 'PATCH026_STATIC_VALIDATION=FAIL reason=%s\n' "$1" >&2
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
        docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md|\
        scripts/validate_patch_026.sh)
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
    docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md \
    scripts/validate_patch_026.sh; do
    printf '%s\n' "$actual_scope" | grep -Fx "$required" >/dev/null || fail "scope_missing:$required"
done

[ "$(git branch --show-current)" = "patch-026-package-3b-requirements-scope-lock" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "76743e137d5d7c446ed4786fd79c798e3e2bc894" ] || fail "base_head"
[ "$(git rev-parse main)" = "76743e137d5d7c446ed4786fd79c798e3e2bc894" ] || fail "local_main"
[ "$(git rev-parse origin/main)" = "76743e137d5d7c446ed4786fd79c798e3e2bc894" ] || fail "origin_main"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

grep -F 'STABLE_REPOSITORY_MERGE=76743e137d5d7c446ed4786fd79c798e3e2bc894' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_merge"
grep -F 'STABLE_IMPLEMENTATION_MERGE=76743e137d5d7c446ed4786fd79c798e3e2bc894' docs/handoff/CURRENT_STATE.md >/dev/null || fail "implementation_merge"
grep -F 'PATCH_025A=COMPLETE_MERGED_SELF_FINALIZING' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch025a_state"
grep -F 'PATCH_025A_PR=36' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch025a_pr"
grep -F 'PATCH_025A_MERGE=76743e137d5d7c446ed4786fd79c798e3e2bc894' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch025a_merge"
grep -F 'PATCH_026=ACTIVE_REQUIREMENTS_SCOPE_LOCK' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch026_state"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=patch-026-package-3b-requirements-scope-lock' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch026_branch"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F 'Package 3B: `NOT_STARTED`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_package3b"
grep -F 'Patch026 - Package 3B Read-Only Requirements and Scope Lock' docs/history/PATCH_HISTORY.md >/dev/null || fail "history_section"
grep -F 'No command is implemented or authorized by Patch026.' docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md >/dev/null || fail "command_boundary"
grep -F 'After Patch026 is merged and verified' docs/history/PATCH_026_PACKAGE_3B_REQUIREMENTS_AND_SCOPE_LOCK.md >/dev/null || fail "post_merge_target"

component_status=$(git status --porcelain -- components managed_components)
[ -z "$component_status" ] || fail "firmware_or_managed_component_changes"

git diff --check >/dev/null 2>&1 || fail "diff_check"

printf 'PATCH026_STATIC_VALIDATION=PASS\n'
printf 'PATCH026_SCOPE=DOCUMENTATION_ONLY\n'
printf 'PACKAGE_3B=NOT_STARTED\n'
printf 'PATCH026_RUNTIME=NOT_APPLICABLE\n'
