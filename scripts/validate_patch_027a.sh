#!/usr/bin/env bash

EXPECTED_SHA="5f79212cda66388b03ecd0be202af0b49e59526d"
EXPECTED_BRANCH="patch-027a-post-merge-state-reconciliation"
ROOT="$(git rev-parse --show-toplevel 2>/dev/null)"

if [ -z "$ROOT" ]; then
  echo "PATCH027A_STATIC_VALIDATION=FAIL: not a git worktree"
  exit 1
fi

cd "$ROOT" || exit 1

actual_branch="$(git branch --show-current)"
actual_sha="$(git rev-parse HEAD 2>/dev/null)"

if [ "$actual_branch" != "$EXPECTED_BRANCH" ]; then
  echo "PATCH027A_STATIC_VALIDATION=FAIL: unexpected branch"
  exit 1
fi

if [ "$actual_sha" != "$EXPECTED_SHA" ]; then
  echo "PATCH027A_STATIC_VALIDATION=FAIL: unexpected base commit"
  exit 1
fi

if [ "$(git diff --cached --name-only | wc -l | tr -d ' ')" != "0" ]; then
  echo "PATCH027A_STATIC_VALIDATION=FAIL: staged changes present"
  exit 1
fi

required_markers="docs/handoff/CURRENT_STATE.md docs/handoff/HANDOFF.md docs/handoff/MASTER_INDEX.md docs/history/PATCH_027_FIRST_PACKAGE_3B_COMMAND_SLICE_SCOPE.md docs/history/PATCH_027A_POST_MERGE_STATE_RECONCILIATION.md"
for path in $required_markers; do
  if [ ! -f "$path" ]; then
    echo "PATCH027A_STATIC_VALIDATION=FAIL: missing $path"
    exit 1
  fi
done

grep -q 'STABLE_REPOSITORY_MERGE=5f79212cda66388b03ecd0be202af0b49e59526d' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'ACTIVE_DEVELOPMENT_PATCH=NONE' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'ACTIVE_DEVELOPMENT_BRANCH=NONE' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'NEXT_FUNCTIONAL_PATCH=PATCH028_HOMEY_STARTUP_STATUS_READINESS_OPTIMIZATION' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'PATCH_027=COMPLETE_MERGED_DOCUMENTATION_ONLY_COMMAND_SLICE_SCOPE' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'PACKAGE_3B_FIRST_USER_COMMAND=NOT_SELECTED' docs/handoff/CURRENT_STATE.md || exit 1
grep -q 'PATCH_013_RUNTIME=NOT_RUN' docs/handoff/CURRENT_STATE.md || exit 1
grep -q '5f79212cda66388b03ecd0be202af0b49e59526d' docs/history/PATCH_027A_POST_MERGE_STATE_RECONCILIATION.md || exit 1

if grep -R -n -E 'components/|managed_components/' docs/history/PATCH_027A_POST_MERGE_STATE_RECONCILIATION.md >/dev/null; then
  echo "PATCH027A_STATIC_VALIDATION=FAIL: firmware scope marker"
  exit 1
fi

echo "PATCH027A_STATIC_VALIDATION=PASS"
echo "PATCH027A_SCOPE=DOCUMENTATION_ONLY"
echo "PATCH027A_BASE=$EXPECTED_SHA"
