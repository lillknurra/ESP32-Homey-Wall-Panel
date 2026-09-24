#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="f111e9f71f3744bb9aafe6dbf8e2e1b829bd458f"
EXPECTED_BRANCH="patch-048-api-version-aware-internet-only-homey-strategy"
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
  docs/history/PATCH_048_API_VERSION_AWARE_INTERNET_ONLY_HOMEY_STRATEGY.md \
  docs/history/PATCH_HISTORY.md \
  scripts/validate_patch_048.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch048 exact eight-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "8" || fail "Patch048 file count is not eight"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch048 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch048 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch048 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch048 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch047A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch047A merge"
test -z "$(git status --porcelain)" || fail "Patch048 validation requires clean worktree"

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts

bash -n scripts/validate_patch_048.sh

require_literal 'PATCH048_API_VERSION_AWARE_REMOTE_STRATEGY_CONTRACT' "$SRC"
require_literal 'api_v2_local_platform_v1: "cloud"' "$SRC"
require_literal 'api_v3_local: "remoteForwarded"' "$SRC"
require_literal 'required_endpoint_evidence: true' "$SRC"
require_literal 'export function resolvePatch048InternetOnlyStrategy' "$SRC"
require_literal 'apiVersion === 2' "$SRC"
require_literal 'platformVersion !== 1' "$SRC"
require_literal 'hasEndpoint("remoteUrl")' "$SRC"
require_literal 'hasEndpoint("remoteUrlForwarded")' "$SRC"
require_literal 'resolvePatch048InternetOnlyStrategy(record, HomeyAPI)' "$SRC"
require_literal 'rather than enabling local fallback' "$SRC"

require_literal 'Patch048 direct pinned runtime selects the source-verified API v2 Internet-only cloud strategy' "$TEST"
require_literal 'Patch048 source-verified strategy mapping covers API v2 and API v3 without local fallback' "$TEST"
require_literal 'Patch048 refuses missing Internet endpoint evidence instead of falling back locally' "$TEST"
require_literal 'Patch048 refuses unsupported Homey API or platform combinations' "$TEST"
require_literal 'assert.deepEqual(observed.strategy, ["cloud"])' "$TEST"

if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then
  fail "mutation or execution surface found"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then
  fail "direct network primitive found"
fi
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then
  fail "local Homey discovery strategy found"
fi

require_literal 'PATCH048_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_048_API_VERSION_AWARE_INTERNET_ONLY_HOMEY_STRATEGY.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH048_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH048_LIVE_ATHOM_ACCESS=NOT_RUN'
printf '%s\n' 'PATCH048_REMOTE_DEVICE_READ=NOT_RUN'
printf '%s\n' 'PATCH048_OAUTH_STORE_WRITE=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH048_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH048_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH048_HOMEY_PAT=NOT_USED_AND_FORBIDDEN'
printf '%s\n' 'PATCH048_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch048 API-version-aware Internet-only Homey strategy offline validation'
