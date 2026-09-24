#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="6be7b5a791f3b65056096406a57ff15b580a5623"
EXPECTED_BRANCH="patch-046-direct-pinned-homey-api-oauth-store-adapter"
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
  docs/history/PATCH_046_DIRECT_PINNED_HOMEY_API_OAUTH_STORE_ADAPTER.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_043_remote_awning_candidates.sh \
  scripts/validate_patch_046.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch046 exact nine-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "9" || fail "Patch046 file count is not nine"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch046 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch046 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch046 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch046 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch045A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch045A merge"
test -z "$(git status --porcelain)" || fail "Patch046 validation requires clean worktree"

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
RUNNER=scripts/run_patch_043_remote_awning_candidates.sh
LOCK=tools/homey-inventory/package-lock.json

bash -n "$RUNNER"
bash -n scripts/validate_patch_046.sh

require_literal '"homey-api": "3.19.1"' tools/homey-inventory/package.json
require_literal '"node_modules/homey-api"' "$LOCK"
require_literal '"version": "3.19.1"' "$LOCK"

require_literal 'PATCH046_DIRECT_PINNED_HOMEY_API_CONTRACT' "$SRC"
require_literal 'package: "homey-api@3.19.1"' "$SRC"
require_literal 'cli_package_dependency: "none"' "$SRC"
require_literal 'oauth_store_write: "forbidden"' "$SRC"
require_literal 'auto_refresh_tokens: false' "$SRC"
require_literal 'createReadOnlyAthomCliOauthStore' "$SRC"
require_literal 'createDirectPinnedHomeyApiRemoteRuntime' "$SRC"
require_literal 'packageJson.version !== "3.19.1"' "$SRC"
require_literal 'autoRefreshTokens: false' "$SRC"
require_literal 'Patch046 refuses OAuth store writes and token refresh persistence' "$SRC"
require_literal 'PATCH046_HOMEY_RUNTIME=DIRECT_PINNED_HOMEY_API__READ_ONLY_OAUTH_STORE' "$RUNNER"

if grep -Fq 'resolveOfficialHomeyCliRoot' "$SRC"; then
  fail "Patch045 CLI package resolver remains on active runtime path"
fi
if grep -Fq 'AthomApiStorage.js' "$SRC"; then
  fail "Patch046 must not depend on Homey CLI AthomApiStorage module"
fi
if grep -Fq 'config.js' "$SRC"; then
  fail "Patch046 must not depend on Homey CLI config module"
fi
if grep -Fq 'clientSecret' "$SRC"; then
  fail "Patch046 must not embed or request OAuth client secrets"
fi
if grep -Fq 'clientId' "$SRC"; then
  fail "Patch046 must not embed or request OAuth client IDs"
fi
if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then
  fail "mutation or execution surface found"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then
  fail "direct network primitive found"
fi
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then
  fail "local Homey discovery strategy found"
fi

require_literal 'Patch046 read-only OAuth store returns only homeyApi and refuses every write' "$TEST"
require_literal 'Patch046 read-only OAuth store rejects permissive files and symlinks' "$TEST"
require_literal 'Patch046 direct pinned runtime has no CLI package dependency' "$TEST"
require_literal 'PATCH046_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_046_DIRECT_PINNED_HOMEY_API_OAUTH_STORE_ADAPTER.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH046_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH046_LIVE_ATHOM_ACCESS=NOT_RUN'
printf '%s\n' 'PATCH046_OAUTH_STORE_WRITE=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH046_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH046_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH046_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch046 direct pinned Homey API OAuth store adapter offline validation'
