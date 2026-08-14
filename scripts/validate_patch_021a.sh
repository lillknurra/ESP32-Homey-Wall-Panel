#!/bin/sh

fail() {
  echo "PATCH021A_VALIDATION=FAIL reason=$1"
  exit 1
}

branch="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)" || fail "git_branch_unavailable"
test "$branch" = "patch-021a-finalize-patch021-post-merge-state" || fail "wrong_branch"

changed="$( (git diff --name-only; git diff --cached --name-only; git ls-files --others --exclude-standard) | sort -u )"
test -n "$changed" || fail "no_patch_changes"

allowed='^(docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md|scripts/validate_patch_021a.sh)$'
unexpected="$(printf '%s\n' "$changed" | grep -Ev "$allowed" || true)"
test -z "$unexpected" || fail "unexpected_scope:$unexpected"

if printf '%s\n' "$changed" | grep -Eq '^(components/|main/|managed_components/|sdkconfig)'; then
  fail "firmware_or_sdkconfig_scope_changed"
fi

grep -F 'STABLE_REPOSITORY_MERGE=bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_repository_merge_missing"
grep -F 'STABLE_IMPLEMENTATION_MERGE=bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_implementation_merge_missing"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch_none_missing"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_branch_none_missing"
grep -F 'NEXT_FUNCTIONAL_PATCH=PATCH021_PASSIVE_RUNTIME_EVIDENCE_COLLECTION' docs/handoff/CURRENT_STATE.md >/dev/null || fail "next_runtime_evidence_missing"
grep -F 'PATCH_021=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021_complete_missing"
grep -F 'PATCH_021_PR=29' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021_pr_missing"
grep -F 'PATCH_021_MERGE=bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021_merge_missing"
grep -F 'PATCH_021_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021_runtime_boundary_missing"
grep -F 'PATCH_021_REMOTE_BRANCH_CLEANUP=COMPLETE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch021_branch_cleanup_missing"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime_changed"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package3b_changed"

grep -F 'Patch021: `COMPLETE / MERGED`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_patch021_complete_missing"
grep -F 'Patch021 PR: `#29`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_patch021_pr_missing"
grep -F 'Patch021 runtime evidence: `NOT_RUN`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_runtime_missing"
grep -F 'Active development patch: `NONE`' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_active_none_missing"
grep -F 'PATCH021_PASSIVE_RUNTIME_EVIDENCE_COLLECTION' docs/handoff/MASTER_INDEX.md >/dev/null || fail "master_index_next_missing"

grep -F 'Patch021: `COMPLETE / MERGED`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_patch021_complete_missing"
grep -F 'Patch021 runtime evidence: `NOT_RUN`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_runtime_missing"
grep -F 'No development patch is active in durable state' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_active_none_missing"
grep -F 'Next evidence scope: `PATCH021_PASSIVE_RUNTIME_EVIDENCE_COLLECTION`' docs/handoff/HANDOFF.md >/dev/null || fail "handoff_next_runtime_missing"

grep -F 'Status: `COMPLETE / MERGED`' docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md >/dev/null || fail "patch_doc_complete_missing"
grep -F 'Pull request: `#29`' docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md >/dev/null || fail "patch_doc_pr_missing"
grep -F 'bb35dfb2c13bd4374996617f5ccb2d4b21d9edb2' docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md >/dev/null || fail "patch_doc_merge_missing"
grep -F 'Flash and runtime validation were `NOT_RUN`' docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md >/dev/null || fail "patch_doc_runtime_missing"

grep -F '## Patch021A - Post-Merge State Reconciliation' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch021a_history_missing"
grep -F 'SELF_FINALIZING' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch021a_self_finalizing_missing"
grep -F 'do not create Patch021B' docs/history/PATCH_HISTORY.md >/dev/null || fail "patch021b_guard_missing"

if grep -R -n 'ACTIVE_DEVELOPMENT_PATCH=PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS\|ACTIVE / DIAGNOSTICS_ONLY / NOT_COMMITTED\|patch-021-homey-remote-panel-ui-responsiveness-diagnostics`.' docs/handoff docs/history/PATCH_021_HOMEY_REMOTE_PANEL_UI_RESPONSIVENESS_DIAGNOSTICS.md docs/history/PATCH_HISTORY.md >/dev/null; then
  fail "stale_patch021_active_state"
fi

echo "PATCH021A_SOURCE_SCOPE=PASS"
echo "PATCH021A_STATE_RECONCILIATION=PASS"
echo "PATCH021A_RUNTIME_BOUNDARY=PASS"
echo "PATCH021A_VALIDATION=PASS"
