#!/usr/bin/env bash
set -u

fail() {
    printf 'PATCH023_STATIC_VALIDATION=FAIL reason=%s\n' "$1" >&2
    exit 1
}

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$repo_dir" || exit 1

is_allowed_scope_path() {
    case "$1" in
        docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md|\
        docs/handoff/CURRENT_STATE.md|\
        docs/handoff/HANDOFF.md|\
        docs/handoff/MASTER_INDEX.md|\
        docs/history/PATCH_HISTORY.md|\
        docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md|\
        docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md|\
        docs/history/PATCH_023_PANEL_UI_RENDER_PATH_REQUIREMENTS.md|\
        scripts/validate_patch_023.sh)
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
    docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md \
    docs/handoff/CURRENT_STATE.md \
    docs/handoff/HANDOFF.md \
    docs/handoff/MASTER_INDEX.md \
    docs/history/PATCH_HISTORY.md \
    docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md \
    docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md \
    docs/history/PATCH_023_PANEL_UI_RENDER_PATH_REQUIREMENTS.md \
    scripts/validate_patch_023.sh; do
    printf '%s\n' "$actual_scope" | grep -Fx "$required" >/dev/null || fail "scope_missing:$required"
done

branch=$(git branch --show-current) || fail "branch_read"
[ "$branch" = "patch-023-bounded-panel-ui-render-path-scope-lock" ] || fail "branch"

head=$(git rev-parse HEAD) || fail "head_read"
[ "$head" = "9de603fd872dceba3fa98ada780fec11eb8dfbe5" ] || fail "base_head"
[ "$(git rev-parse main)" = "$head" ] || fail "local_main"
[ "$(git rev-parse origin/main)" = "$head" ] || fail "origin_main"

grep -F 'STABLE_REPOSITORY_MERGE=9de603fd872dceba3fa98ada780fec11eb8dfbe5' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_merge"
grep -F 'PATCH_022A=COMPLETE_MERGED_SELF_FINALIZING' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022a_state"
grep -F 'PATCH_022A_PR=32' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022a_pr"
grep -F 'PATCH_022A_MERGE=9de603fd872dceba3fa98ada780fec11eb8dfbe5' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022a_merge"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH023_PANEL_UI_RENDER_PATH_REQUIREMENTS' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch023_active"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=patch-023-bounded-panel-ui-render-path-scope-lock' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch023_branch"
grep -F 'Package 3B: `NOT_STARTED`' docs/handoff/HANDOFF.md >/dev/null || fail "package3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F 'Patch023 UI render-path requirements and scope lock' docs/architecture/DISPLAY_UX_AND_CONTROL_ARCHITECTURE.md >/dev/null || fail "architecture_section"
grep -F 'No `components/**` file is in scope.' docs/history/PATCH_023_PANEL_UI_RENDER_PATH_REQUIREMENTS.md >/dev/null || fail "component_boundary"
grep -F 'Patch022B' docs/handoff/HANDOFF.md >/dev/null || fail "recursive_finalization_language_missing"

if grep -F 'PATCH022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION' docs/handoff/CURRENT_STATE.md >/dev/null; then
    fail "stale_active_patch022a"
fi

component_status=$(git status --porcelain -- components)
[ -z "$component_status" ] || fail "component_changes"

git diff --check >/dev/null 2>&1 || fail "diff_check"

printf 'PATCH023_STATIC_VALIDATION=PASS\n'
