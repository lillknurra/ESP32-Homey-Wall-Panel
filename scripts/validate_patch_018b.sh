#!/bin/sh

fail() {
  printf 'PATCH018B_VALIDATION=FAIL %s\n' "$1" >&2
  exit 1
}

current_branch=$(git branch --show-current) || fail "branch_check"
test "$current_branch" = "patch-018b-finalize-patch018a-post-merge-state" || fail "wrong_branch"

head_sha=$(git rev-parse HEAD) || fail "head_check"
test "$head_sha" = "481897cead752f8f6bf8ebc18b059845d7fc9ac0" || fail "wrong_head"

main_sha=$(git rev-parse main) || fail "main_check"
origin_main_sha=$(git rev-parse origin/main) || fail "origin_main_check"
test "$main_sha" = "481897cead752f8f6bf8ebc18b059845d7fc9ac0" || fail "wrong_main"
test "$origin_main_sha" = "481897cead752f8f6bf8ebc18b059845d7fc9ac0" || fail "wrong_origin_main"

staged=$(git diff --cached --name-only) || fail "staged_check"
test -z "$staged" || fail "staged_files_present"

allowed='^(docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md|docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md|scripts/validate_patch_018b.sh)$'

tracked_changed=$(git diff --name-only -- .) || fail "changed_files"
untracked_changed=$(git ls-files --others --exclude-standard) || fail "untracked_files"
changed=$(printf '%s\n%s\n' "$tracked_changed" "$untracked_changed")
unexpected=$(printf '%s\n' "$changed" | grep -v '^$' | grep -Ev "$allowed" || true)
test -z "$unexpected" || fail "unexpected_files"

components_changed=$(printf '%s\n' "$changed" | grep '^components/' || true)
test -z "$components_changed" || fail "components_scope_changed"

test -f docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md || fail "history_file_missing"
test -f scripts/validate_patch_018b.sh || fail "validator_file_missing"

grep -F 'STABLE_REPOSITORY_MERGE=481897cead752f8f6bf8ebc18b059845d7fc9ac0' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_repository_merge_missing"
grep -F 'STABLE_IMPLEMENTATION_MERGE=481897cead752f8f6bf8ebc18b059845d7fc9ac0' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_implementation_merge_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch_none_missing"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_branch_none_missing"
grep -F 'NEXT_FUNCTIONAL_PATCH=UNDECIDED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "next_patch_undecided_missing"
grep -F 'PATCH_018A=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch018a_complete_missing"
grep -F 'PATCH_018A_MERGE=481897cead752f8f6bf8ebc18b059845d7fc9ac0' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch018a_merge_missing"
grep -F 'PATCH_018A_REMOTE_BRANCH_CLEANUP=COMPLETE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch018a_cleanup_missing"
grep -F 'PATCH_018B=SELF_FINALIZING_DOCUMENTATION_ONLY' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch018b_state_missing"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime_boundary_missing"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_boundary_missing"

grep -F 'docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_history_missing"
grep -F 'Active development patch: `NONE`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_active_none_missing"
grep -F 'Next functional patch: `UNDECIDED`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_next_undecided_missing"
grep -F 'Patch018B is self-finalizing documentation-only reconciliation' docs/handoff/MASTER_INDEX.md >/dev/null || fail "index_self_finalizing_missing"

grep -F 'No development patch is active in durable state' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_active_none_missing"
grep -F 'do not create Patch018C solely' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_no_patch018c_missing"

grep -F 'Patch018A - Panel UI Swipe Diagnostic Resolution' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018a_history_missing"
grep -F 'Squash merge commit:' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018a_merge_label_missing"
grep -F '481897cead752f8f6bf8ebc18b059845d7fc9ac0' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018a_merge_sha_missing"
grep -F 'Patch018B - Post-Merge State Reconciliation' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018b_history_missing"
grep -F 'SELF_FINALIZING' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch018b_self_finalizing_missing"

grep -F 'Status: `COMPLETE / MERGED`' docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md >/dev/null || fail "patch018a_doc_status_missing"
grep -F 'Pull request: `#27`' docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md >/dev/null || fail "patch018a_doc_pr_missing"
grep -F '481897cead752f8f6bf8ebc18b059845d7fc9ac0' docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md >/dev/null || fail "patch018a_doc_merge_missing"
grep -F 'Do not promote that host-test failure to' docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md >/dev/null || fail "hosttest_boundary_missing"

grep -F 'Status: `ACTIVE / DOCUMENTATION_ONLY / SELF_FINALIZING / NOT_COMMITTED`' docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md >/dev/null || fail "patch018b_doc_status_missing"
grep -F 'Any `components/**` change.' docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md >/dev/null || fail "patch018b_forbidden_components_missing"
grep -F 'no Patch018C or other' docs/history/PATCH_018B_POST_MERGE_STATE_RECONCILIATION.md >/dev/null || fail "patch018b_no_patch018c_missing"

if grep -R -n 'Patch018A is the only active development patch\|ACTIVE_DEVELOPMENT_PATCH=PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION\|ACTIVE_DEVELOPMENT_BRANCH=patch-018a-panel-ui-swipe-diagnostic-resolution' docs/handoff docs/history/PATCH_018A_PANEL_UI_SWIPE_DIAGNOSTIC_RESOLUTION.md docs/history/PATCH_HISTORY.md >/dev/null; then
  fail "stale_patch018a_active_status"
fi

if git diff -- docs/handoff docs/history scripts | grep -E '(access_token|refresh_token|Authorization: Bearer|client_secret)[[:space:]]*[:=][[:space:]]*[^`[:space:]]' >/dev/null; then
  fail "secret_pattern"
fi

if git diff -- docs/handoff docs/history scripts | grep -E '(setCapabilityValue|triggerFlow|runFlow|activateMood)' >/dev/null; then
  fail "mutation_pattern"
fi

printf 'PATCH018B_VALIDATION=PASS\n'
