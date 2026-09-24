#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="9359dab6143cb42a2fdb0a0a4478cf996b3b2b69"
EXPECTED_BRANCH="patch-044-no-side-effect-athom-oauth-session-gate"
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
  docs/history/PATCH_044_NO_SIDE_EFFECT_ATHOM_OAUTH_SESSION_GATE.md \
  docs/history/PATCH_HISTORY.md \
  scripts/validate_patch_044.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch044 exact eight-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "8" || fail "Patch044 file count is not eight"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch044 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch044 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch044 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch044 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch043A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch043A merge"
test -z "$(git status --porcelain)" || fail "Patch044 validation requires clean worktree"

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig)' "$ACTUAL"; then
  fail "firmware/config scope detected"
fi

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
bash -n scripts/validate_patch_044.sh

require_literal 'PATCH044_NO_LOGIN_OAUTH_GATE' "$SRC"
require_literal 'listStoredOauthHomeysNoLogin' "$SRC"
require_literal 'AthomCloudAPI' "$SRC"
require_literal './lib/AthomApiStorage.js' "$SRC"
require_literal 'cloud.isLoggedIn()' "$SRC"
require_literal 'cloud.getAuthenticatedUser()' "$SRC"
require_literal 'user.getHomeys()' "$SRC"
require_literal 'browser OAuth is required' "$SRC"

if grep -Fq './services/AthomApi.js' "$SRC"; then
  fail "Patch044 source must not import the Homey CLI AthomApi wrapper"
fi
if grep -nE '\.(login|authenticateWithAuthorizationCode)[[:space:]]*\(' "$SRC"; then
  fail "browser/OAuth login side-effect call found in Patch044 source"
fi
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then
  fail "local discovery strategy found"
fi
if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then
  fail "mutation or execution surface found"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then
  fail "direct network primitive found"
fi

require_literal 'Patch044 no-login OAuth gate refuses absent stored session' "$TEST"
require_literal 'without invoking login' "$TEST"
require_literal 'authenticated-user errors without login fallback' "$TEST"

require_literal 'ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=PATCH044' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH044_BROWSER_LOGIN_SIDE_EFFECT=FORBIDDEN' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_044_NO_SIDE_EFFECT_ATHOM_OAUTH_SESSION_GATE.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH044_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH044_LIVE_ATHOM_ACCESS=NOT_RUN'
printf '%s\n' 'PATCH044_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH044_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH044_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch044 no-side-effect Athom OAuth session gate offline validation'
