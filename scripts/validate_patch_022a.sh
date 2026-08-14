#!/bin/sh

fail() {
  echo "PATCH022A_VALIDATION=FAIL reason=$1"
  exit 1
}

branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)" || fail "git_branch_unavailable"
test "$branch" = "patch-022a-finalize-patch022-post-merge-runtime-evidence" || fail "wrong_branch"

changed="$( (git diff --name-only; git diff --cached --name-only; git ls-files --others --exclude-standard) | sort -u )"
test -n "$changed" || fail "no_patch_changes"

allowed='^(docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md|docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md|scripts/validate_patch_022a.sh)$'
unexpected="$(printf '%s\n' "$changed" | grep -Ev "$allowed" || true)"
test -z "$unexpected" || fail "unexpected_scope:$unexpected"

if printf '%s\n' "$changed" | grep -Eq '^(components/|main/|managed_components/|sdkconfig|build/)'; then
  fail "firmware_or_sdkconfig_scope_changed"
fi

grep -F 'STABLE_REPOSITORY_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_repository_merge_missing"
grep -F 'STABLE_IMPLEMENTATION_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_implementation_merge_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=PATCH022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch_missing"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=patch-022a-finalize-patch022-post-merge-runtime-evidence' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_branch_missing"
grep -F 'NEXT_FUNCTIONAL_PATCH=PATCH022_UI_OPTIMIZATION_SCOPE_ANALYSIS' docs/handoff/CURRENT_STATE.md >/dev/null || fail "next_scope_missing"
grep -F 'PATCH_022=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022_complete_missing"
grep -F 'PATCH_022_PR=31' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022_pr_missing"
grep -F 'PATCH_022_MERGE=3cb8993df9a67e105cf70213ac9a5510e32d73dd' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022_merge_missing"
grep -F 'PATCH_022_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_PATH' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022_runtime_missing"
grep -F 'PATCH_022A=ACTIVE_DOCUMENTATION_ONLY' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch022a_state_missing"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime_changed"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_changed"

grep -F 'Patch022: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_PATH`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_patch022_missing"
grep -F 'Patch022 PR: `#31`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_pr_missing"
grep -F 'PATCH022_UI_OPTIMIZATION_SCOPE_ANALYSIS' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_next_missing"

grep -F 'Patch022: `COMPLETE / MERGED`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_patch022_missing"
grep -F 'Patch022A: `ACTIVE / DOCUMENTATION_ONLY / SELF_FINALIZING`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_patch022a_missing"
grep -F 'PATCH022_UI_OPTIMIZATION_SCOPE_ANALYSIS' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_next_missing"

grep -F 'Status: `COMPLETE / MERGED / RUNTIME_ACCEPTED_FOR_OBSERVED_PATH`' docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null || fail "patch_doc_status_missing"
grep -F 'Pull request: `#31`' docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null || fail "patch_doc_pr_missing"
grep -F 'passive UI runtime for observed path: `PASS`' docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null || fail "patch_doc_runtime_missing"

grep -F '## Patch022A - Finalize Patch022 Post-Merge Runtime Evidence' docs/history/PATCH_HISTORY.md >/dev/null || fail "history_patch022a_missing"
grep -F 'ACTIVE / DOCUMENTATION_ONLY / SELF_FINALIZING' docs/history/PATCH_HISTORY.md >/dev/null || fail "history_patch022a_status_missing"
grep -F 'do not create Patch022B' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch022b_guard_missing"

grep -F 'Patch022A - Post-Merge Runtime Evidence Reconciliation' docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "patch022a_doc_missing"
grep -F '2a1b43ff68594816af793cd8892a570baca9f3ba58e00537bab70a9b6ed82207' docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "runtime_log_hash_missing"
grep -F 'f47ae63dcd029465d3442b6323ad94e7eaa5392587ffdf017e1da216cb0254d1' docs/history/PATCH_022A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "runtime_summary_hash_missing"

if grep -R -n 'ACTIVE_DEVELOPMENT_PATCH=PATCH022_BOUNDED_PANEL_UI_SCROLL_RESPONSIVENESS\|PATCH_022=ACTIVE_NOT_COMMITTED\|Status: `ACTIVE / IMPLEMENTATION / NOT_COMMITTED`' docs/handoff docs/history/PATCH_022_PANEL_UI_SCROLL_RESPONSIVENESS.md >/dev/null; then
  fail "stale_patch022_active_state"
fi

echo "PATCH022A_SOURCE_SCOPE=PASS"
echo "PATCH022A_STATE_RECONCILIATION=PASS"
echo "PATCH022A_RUNTIME_EVIDENCE_REGISTRATION=PASS"
echo "PATCH022A_VALIDATION=PASS"
