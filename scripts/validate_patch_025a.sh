#!/usr/bin/env bash

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fail() {
    printf 'PATCH025A_STATIC=FAIL reason=%s\n' "$1"
    exit 1
}

cd "$ROOT" || fail "repository_unavailable"

[ "$(git branch --show-current)" = "patch-025a-finalize-patch025-post-merge-runtime-evidence" ] || fail "branch"
[ "$(git rev-parse HEAD)" = "42c805039e60a2d6a033ef9d8f225369f5062457" ] || fail "base_head"
[ "$(git diff --cached --name-only | wc -l | tr -d ' ')" = "0" ] || fail "staged_changes"

allowed='docs/handoff/MASTER_INDEX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_HISTORY.md
docs/history/PATCH_025_BOUNDED_HOMEY_INVENTORY_SCHEMA_DIAGNOSTICS.md
docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md
scripts/validate_patch_025a.sh'
actual="$(git status --short | sed -E 's/^.. //')"
if [ "$(printf '%s\n' "$actual" | sort)" != "$(printf '%s\n' "$allowed" | sort)" ]; then
    fail "exact_scope"
fi

forbidden='components/ managed_components/ sdkconfig'
for token in $forbidden; do
    git status --short | grep -F "$token" >/dev/null && fail "forbidden_scope"
done

grep -F 'PATCH_025=COMPLETE_MERGED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch025_state"
grep -F 'PATCH_025_RUNTIME=PASS_EXTERNAL_EVIDENCE_OBSERVED_STARTUP_PATH' docs/handoff/CURRENT_STATE.md >/dev/null || fail "runtime_state"
grep -F 'PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION' docs/handoff/MASTER_INDEX.md >/dev/null || fail "active_patch"
grep -F '42c805039e60a2d6a033ef9d8f225369f5062457' docs/handoff/HANDOFF.md >/dev/null || fail "merge_sha"
grep -F 'PACKAGE_3B=NOT_STARTED' docs/handoff/CURRENT_STATE.md >/dev/null || fail "package_3b_boundary"
grep -F 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md >/dev/null || fail "patch013_boundary"
grep -F '6b85061334aaa25d37792fa0b1be5660b9e1c52fc21735f8f8a642c9cfb9dfc3' docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "log_sha"
grep -F '22f94b777f09bc0c6f3e859859281da21ef115034bd49f676832fec4707fb1d8' docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "summary_sha"
grep -F 'PATCH025V2_RUNTIME_ACCEPTANCE=PASS' docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "runtime_acceptance"
grep -F 'exact one-summary-per-fetch runtime count: `NOT_OBSERVED`' docs/history/PATCH_025A_POST_MERGE_RUNTIME_EVIDENCE_RECONCILIATION.md >/dev/null || fail "not_observed_boundary"

git diff --check || fail "diff_check"
printf 'PATCH025A_STATIC=PASS\n'
printf 'PATCH025A_SCOPE=DOCUMENTATION_ONLY\n'
printf 'PATCH025A_RUNTIME=NOT_APPLICABLE\n'
