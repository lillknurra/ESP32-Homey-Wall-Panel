#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="26e63256748b0c33044585edde274c17e73d35b1"
EXPECTED_BRANCH="patch-045-official-homey-cli-installation-resolver"
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
  docs/history/PATCH_045_OFFICIAL_HOMEY_CLI_INSTALLATION_RESOLVER.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_043_remote_awning_candidates.sh \
  scripts/validate_patch_045.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch045 exact nine-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "9" || fail "Patch045 file count is not nine"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch045 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch045 base is not an ancestor"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch045 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits forbidden in Patch045 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch044A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch044A merge"
test -z "$(git status --porcelain)" || fail "Patch045 validation requires clean worktree"

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"

SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
RUNNER=scripts/run_patch_043_remote_awning_candidates.sh

bash -n "$RUNNER"
bash -n scripts/validate_patch_045.sh

require_literal 'resolveOfficialHomeyCliRootFromCandidates' "$SRC"
require_literal 'execFileSync("which", ["homey"]' "$SRC"
require_literal 'execFileSync("npm", ["root", "-g"]' "$SRC"
require_literal '.nvm/versions/node' "$SRC"
require_literal '.local/share/fnm/node-versions' "$SRC"
require_literal '.asdf/installs/nodejs' "$SRC"
require_literal '/opt/homebrew/lib/node_modules/homey' "$SRC"
require_literal '/usr/local/lib/node_modules/homey' "$SRC"
require_literal 'node_modules/homey-api/package.json' "$SRC"
require_literal 'PATCH045_HOMEY_CLI_RESOLUTION=PRODUCT_RUNTIME_MULTI_ROOT_FAIL_CLOSED' "$RUNNER"

if grep -Fq 'HOMEY_CLI_ROOT="${PATCH043_HOMEY_CLI_ROOT:-$(npm root -g)/homey}"' "$RUNNER"; then
  fail "runner still hardcodes current npm global root"
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

require_literal 'Patch045 resolver skips incompatible npm-root candidate' "$TEST"
require_literal 'Patch045 resolver canonicalizes a Homey CLI symlink target' "$TEST"
require_literal 'PATCH045_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH_045_OFFICIAL_HOMEY_CLI_INSTALLATION_RESOLVER.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --
(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH045_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH045_LIVE_ATHOM_ACCESS=NOT_RUN'
printf '%s\n' 'PATCH045_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH045_LOCAL_DISCOVERY=NOT_RUN_AND_FORBIDDEN'
printf '%s\n' 'PATCH045_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch045 official Homey CLI installation resolver offline validation'
