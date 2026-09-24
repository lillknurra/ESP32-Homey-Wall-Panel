#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="90bb7463e1e3d0963ff6c1c6c560331608e31990"
EXPECTED_BRANCH="patch-043-athom-oauth-remote-only-awning-candidate-discovery"
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }
require_literal() { grep -Fq -- "$1" "$2" || fail "missing literal in $2: $1"; }

EXPECTED="$(mktemp)"
ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL"' EXIT

printf '%s\n' \
  docs/handoff/CURRENT_STATE.md \
  docs/handoff/HANDOFF.md \
  docs/handoff/MASTER_INDEX.md \
  docs/history/PATCH_043_ATHOM_OAUTH_REMOTE_ONLY_AWNING_CANDIDATE_DISCOVERY.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_043_remote_awning_candidates.sh \
  scripts/validate_patch_043.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch043 exact nine-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "9" || fail "Patch043 file count is not nine"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch043 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch043 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch043 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch043 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch042A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch042A merge"
test -z "$(git status --porcelain)" || fail "Patch043 validation requires clean worktree"

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig)' "$ACTUAL"; then
  fail "firmware/config scope detected"
fi

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
RUNNER=scripts/run_patch_043_remote_awning_candidates.sh

bash -n "$RUNNER"
bash -n scripts/validate_patch_043.sh

require_literal 'getHomeys({ cache: false, local: false })' "$SRC"
require_literal 'REMOTE_FORWARDED' "$SRC"
require_literal 'DISCOVERY_STRATEGIES.CLOUD' "$SRC"
require_literal 'strategy: [requestedStrategy]' "$SRC"
require_literal 'ManagerDevices.getDevices' "$SRC"
require_literal 'Patch043 refuses HOMEY_PAT' "$SRC"
require_literal 'PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN' "$SRC"
require_literal 'PATCH043_HOMEY_PAT=NOT_USED_AND_FORBIDDEN' "$SRC"
require_literal 'HOMEY_PAT/token mode' "$RUNNER"

if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then
  fail "mutation or execution surface found"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then
  fail "direct network primitive found; Patch043 must delegate to official Athom/Homey API"
fi
if grep -nE 'getHomeys\(\{[^}]*local:[[:space:]]*true|DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then
  fail "local discovery strategy found in Patch043 source"
fi

require_literal 'Patch043 contract is Athom Internet-only and mutation-free' "$TEST"
require_literal 'getHomeysRemoteOnly' "$TEST"
require_literal 'exactly one devices read' "$TEST"
require_literal 'Homey Cloud strategy remains Internet-only' "$TEST"

require_literal 'ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=PATCH043' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH043_INTERNET_ONLY_POLICY=ATHOM_API_ONLY__NO_LOCAL_FALLBACK' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_043_ATHOM_OAUTH_REMOTE_ONLY_AWNING_CANDIDATE_DISCOVERY.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH043_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH043_LIVE_ATHOM_OAUTH=NOT_RUN'
printf '%s\n' 'PATCH043_REMOTE_HOMEY_LIST=NOT_RUN'
printf '%s\n' 'PATCH043_REMOTE_DEVICE_READ=NOT_RUN'
printf '%s\n' 'PATCH043_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH043_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch043 Athom OAuth remote-only awning candidate discovery offline validation'
