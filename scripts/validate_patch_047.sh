#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="5f748b31ba38b93f39bf6d728c3911a06253993e"
EXPECTED_BRANCH="patch-047-storage-adapter-inheritance-closure"
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
  docs/history/PATCH_047_STORAGE_ADAPTER_INHERITANCE_CLOSURE.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_043_remote_awning_candidates.sh \
  scripts/validate_patch_047.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch047 exact nine-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "9" || fail "Patch047 file count is not nine"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch047 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch047 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch047 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch047 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch046A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch046A merge"
test -z "$(git status --porcelain)" || fail "Patch047 validation requires clean worktree"

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
RUNNER=scripts/run_patch_043_remote_awning_candidates.sh

bash -n "$RUNNER"
bash -n scripts/validate_patch_047.sh

require_literal 'PATCH047_STORAGE_ADAPTER_INHERITANCE_CONTRACT' "$SRC"
require_literal 'storage_adapter_base: "AthomCloudAPI.StorageAdapter"' "$SRC"
require_literal 'StorageAdapter: Patch047StorageAdapterConstructor' "$SRC"
require_literal 'class ReadOnlyAthomCliOauthStore extends StorageAdapterBase' "$SRC"
require_literal 'homeyApiModule.AthomCloudAPI.StorageAdapter' "$SRC"
require_literal 'typeof module.AthomCloudAPI.StorageAdapter !== "function"' "$SRC"
require_literal 'Patch047 refuses OAuth store writes and token refresh persistence' "$SRC"
require_literal 'autoRefreshTokens: false' "$SRC"
require_literal 'PATCH047_STORAGE_ADAPTER=ATHOMCLOUDAPI_STORAGEADAPTER_SUBCLASS' "$RUNNER"

require_literal 'Invalid store. Must extend AthomCloudAPI/StorageAdapter.' "$TEST"
require_literal 'Patch047 read-only OAuth store inherits the supplied AthomCloudAPI StorageAdapter base' "$TEST"
require_literal 'store instanceof FakeStorageAdapter' "$TEST"

if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then
  fail "mutation or execution surface found"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then
  fail "direct network primitive found"
fi
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then
  fail "local Homey discovery strategy found"
fi

require_literal 'PATCH047_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_047_STORAGE_ADAPTER_INHERITANCE_CLOSURE.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH047_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH047_LIVE_ATHOM_ACCESS=NOT_RUN'
printf '%s\n' 'PATCH047_OAUTH_STORE_WRITE=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH047_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH047_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH047_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch047 StorageAdapter inheritance closure offline validation'
