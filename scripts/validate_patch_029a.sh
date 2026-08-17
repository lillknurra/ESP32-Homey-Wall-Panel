#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH029A_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-029a-finalize-patch029-post-merge-runtime-evidence" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "9a1278ba4b27f7b05e21ef172cefe09ffcb87c09" ] || fail "base_head"
[ -z "$(git diff --cached --name-only)" ] || fail "staged_changes"

allowed='docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_029_HOMEY_FAVORITES_STATUS_BINDING.md
docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md
scripts/validate_patch_029a.sh'

actual="$(git status --short | sed -E 's/^.. //')"
while IFS= read -r path; do
    [ -z "$path" ] && continue
    printf '%s\n' "$allowed" | grep -Fx "$path" >/dev/null || fail "scope_$path"
done <<EOF
$actual
EOF

grep -F 'STABLE_REPOSITORY_MERGE=9a1278ba4b27f7b05e21ef172cefe09ffcb87c09' docs/handoff/CURRENT_STATE.md >/dev/null || fail "stable_merge"
grep -F 'ACTIVE_DEVELOPMENT_PATCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_patch"
grep -F 'ACTIVE_DEVELOPMENT_BRANCH=NONE' docs/handoff/CURRENT_STATE.md >/dev/null || fail "active_branch"
grep -F 'NEXT_FUNCTIONAL_PATCH=UNDECIDED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "next_scope"
grep -F 'PATCH_029=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch029_state"
grep -F 'PATCH_029_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_VALID_CONFIGURED_PATH' docs/handoff/CURRENT_STATE.md >/dev/null || fail "runtime_state"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b"
grep -F 'PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "command_state"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_runtime"
grep -F 'Patch029 - Homey Favorites Validation and Light Status Binding' docs/history/PATCH_029_HOMEY_FAVORITES_STATUS_BINDING.md >/dev/null || fail "patch029_history"
grep -F '9a1278ba4b27f7b05e21ef172cefe09ffcb87c09' docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "reconciliation_merge"
grep -F '151b3678d8d1b1d9577f202d0c5bc385bdfa6a4c2eb11c547ca07f199ae19bfc' docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "evidence_log_sha"
grep -F '7064c47d5c257efc0a88949868f849604c5a8b48eea830c73d1d9a15bea0aa8f' docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "evidence_summary_sha"
grep -F 'VALID_EMPTY' docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "empty_state"
grep -F 'NOT_OBSERVED' docs/history/PATCH_029A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "not_observed_state"

if git status --short | grep -E '(^|[[:space:]])(components/|managed_components/|sdkconfig)' >/dev/null; then
    fail "forbidden_firmware_scope"
fi

git diff --check || fail "diff_check"

printf 'PATCH029A_STATIC=PASS\n'
printf 'PATCH029A_SCOPE=DOCUMENTATION_ONLY\n'
printf 'PATCH029A_RUNTIME=NOT_APPLICABLE\n'
