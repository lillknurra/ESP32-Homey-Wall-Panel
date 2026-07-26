#!/usr/bin/env bash
set -euo pipefail

REPO="${HOME}/GitHub/ESP32-Homey-Wall-Panel"
BASELINE="ac56dec830ebb15e83195bd0eea9875b93966983"
BRANCH="patch-008-workflow-packaging-validation-hardening"

cd "$REPO"

fail() { printf 'FAIL: %s\n' "$1" >&2; exit 1; }
pass() { printf 'PASS: %s\n' "$1"; }
require_file() { test -f "$1" || fail "missing file $1"; }
require_fixed() {
  local file="$1" marker="$2"
  grep -Fq -- "$marker" "$file" || fail "missing marker in $file: $marker"
}
reject_fixed() {
  local file="$1" marker="$2"
  if grep -Fq -- "$marker" "$file"; then fail "stale or forbidden marker in $file: $marker"; fi
}

actual_branch="$(git branch --show-current)"
test "$actual_branch" = "$BRANCH" || fail "expected branch $BRANCH, got $actual_branch"
test "$(git rev-parse HEAD)" = "$BASELINE" || fail "HEAD is not the approved baseline"
test "$(git merge-base HEAD origin/main)" = "$BASELINE" || fail "origin/main merge base changed"

expected="$(mktemp)"
actual="$(mktemp)"
trap 'rm -f "$expected" "$actual"' EXIT
printf '%s\n' \
  AGENTS.md \
  PROJECT_INSTRUCTIONS.md \
  docs/development/DEVELOPMENT_WORKFLOW.md \
  docs/development/VALIDATION_WORKFLOW.md \
  docs/handoff/MASTER_INDEX.md \
  docs/handoff/CURRENT_STATE.md \
  docs/handoff/HANDOFF.md \
  docs/history/PATCH_HISTORY.md \
  docs/history/PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md \
  scripts/validate_patch_008.sh | sort > "$expected"
{
  git diff --name-only "$BASELINE"
  git ls-files --others --exclude-standard
} | sed '/^$/d' | sort -u > "$actual"
cmp -s "$expected" "$actual" || {
  printf 'Expected scope:\n'; cat "$expected"
  printf 'Actual scope:\n'; cat "$actual"
  fail "Patch 008 scope mismatch"
}
pass "exact ten-file scope"

while IFS= read -r file; do require_file "$file"; done < "$expected"

if grep -E '^(components/|main/|sdkconfig|CMakeLists\.txt)' "$actual" >/dev/null; then
  fail "firmware, component, configuration or build file entered documentation-only scope"
fi
pass "documentation-only boundary"

bash -n scripts/validate_patch_008.sh
pass "validator shell syntax"

require_fixed AGENTS.md "## Operator communication and failure handling"
require_fixed AGENTS.md "## Nästa prompt"
require_fixed AGENTS.md "unambiguous"
require_fixed AGENTS.md "smallest safe correction"
require_fixed AGENTS.md "only when the supplied evidence is insufficient"

require_fixed PROJECT_INSTRUCTIONS.md 'ESP-IDF baseline: `v6.0.1`'
require_fixed PROJECT_INSTRUCTIONS.md 'ESP-IDF path: `~/GitHub/esp-idf-v6.0.1`'
require_fixed PROJECT_INSTRUCTIONS.md "does not provide a physical reset-button workflow"
require_fixed PROJECT_INSTRUCTIONS.md 'must not depend on `Ctrl+]`'
require_fixed PROJECT_INSTRUCTIONS.md "must not use shell heredocs"
require_fixed PROJECT_INSTRUCTIONS.md '`~/Downloads/`'

require_fixed docs/development/DEVELOPMENT_WORKFLOW.md "## Proportional controls"
require_fixed docs/development/DEVELOPMENT_WORKFLOW.md '`apply_validate_and_collect.py`'
require_fixed docs/development/DEVELOPMENT_WORKFLOW.md '`mktemp`'
require_fixed docs/development/DEVELOPMENT_WORKFLOW.md "one timestamped result ZIP"
require_fixed docs/development/DEVELOPMENT_WORKFLOW.md "separate explicitly approved phase"

require_fixed docs/development/VALIDATION_WORKFLOW.md "## Actual source representation"
require_fixed docs/development/VALIDATION_WORKFLOW.md "adjacent C string literals"
require_fixed docs/development/VALIDATION_WORKFLOW.md "brittle long exact-text matching"
require_fixed docs/development/VALIDATION_WORKFLOW.md "BEFORE"
require_fixed docs/development/VALIDATION_WORKFLOW.md "AFTER"
require_fixed docs/development/VALIDATION_WORKFLOW.md "unknown hash"
require_fixed docs/development/VALIDATION_WORKFLOW.md "idempotent"
require_fixed docs/development/VALIDATION_WORKFLOW.md "15 seconds"
require_fixed docs/development/VALIDATION_WORKFLOW.md '`Ctrl+]`'
require_fixed docs/development/VALIDATION_WORKFLOW.md '`~/Downloads/`'
require_fixed docs/development/VALIDATION_WORKFLOW.md "operator-observed"
reject_fixed docs/development/VALIDATION_WORKFLOW.md 'untracked `artifacts/` directory'

require_fixed docs/handoff/MASTER_INDEX.md "PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md"
require_fixed docs/handoff/CURRENT_STATE.md 'Current stable baseline: `ac56dec830ebb15e83195bd0eea9875b93966983`'
require_fixed docs/handoff/CURRENT_STATE.md 'Latest completed and merged patch: `Patch 007'
require_fixed docs/handoff/CURRENT_STATE.md 'Active patch: `Patch 008'
require_fixed docs/handoff/HANDOFF.md "Patch 007 is complete and merged through PR #9"
require_fixed docs/history/PATCH_HISTORY.md "## Patch 008 - Workflow, Packaging and Validation Hardening"
require_fixed docs/history/PATCH_HISTORY.md '`1e32fc0ae69f37da55161a416f894394403ecede`'
require_fixed docs/history/PATCH_HISTORY.md '`ac56dec830ebb15e83195bd0eea9875b93966983`'

reject_fixed docs/handoff/CURRENT_STATE.md 'Active branch: `patch-007-persistent-wifi-reconnect`'
reject_fixed docs/handoff/CURRENT_STATE.md "no Patch 007 commit, push, pull request or merge exists yet"
reject_fixed docs/handoff/HANDOFF.md "Patch 007 is the active firmware patch"
reject_fixed docs/handoff/HANDOFF.md "prepare the exact-path commit"

# Permit policy words in prose, but reject likely concrete assignments.
if grep -ERn --exclude='validate_patch_008.sh' \
  '(^|[^A-Za-z])(PASSWORD|PASSWD|PSK|TOKEN|API_KEY|SECRET)[[:space:]]*=[[:space:]]*[^[:space:]`"]+' \
  AGENTS.md PROJECT_INSTRUCTIONS.md docs/development docs/handoff docs/history/PATCH_HISTORY.md \
  docs/history/PATCH_008_WORKFLOW_PACKAGING_AND_VALIDATION_HARDENING.md; then
  fail "possible sensitive value assignment"
fi
pass "sensitive-value assignment scan"

git diff --check "$BASELINE"
pass "git diff --check"

pass "Patch 008 documentation, packaging and validation hardening"
printf 'RESULT=PASS\n'
