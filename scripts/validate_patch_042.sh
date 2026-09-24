#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="85ff9d0a5da95ac086ae396c6cf16ce0b02961f3"
EXPECTED_BRANCH="patch-042-live-strict-local-read-only-awning-evidence-capture"
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

require_literal() {
  local pattern="$1"
  local path="$2"
  grep -Fq -- "$pattern" "$path" || fail "missing required literal in $path: $pattern"
}

EXPECTED="$(mktemp)"
ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL"' EXIT

printf '%s\n' \
  docs/handoff/CURRENT_STATE.md \
  docs/handoff/HANDOFF.md \
  docs/handoff/MASTER_INDEX.md \
  docs/history/PATCH_042_LIVE_STRICT_LOCAL_READ_ONLY_AWNING_EVIDENCE_CAPTURE_RUNNER.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_042_read_only_awning_capture.sh \
  scripts/validate_patch_042.sh \
  tools/homey-inventory/src/awning-live-capture.ts \
  tools/homey-inventory/src/awning-private-selection.ts \
  tools/homey-inventory/test/awning-live-capture.test.ts \
  tools/homey-inventory/test/awning-private-selection.test.ts | sort > "$EXPECTED"

git diff --name-only "$EXPECTED_BASE" -- | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch042 exact eleven-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "11" || fail "Patch042 file count is not eleven"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected Patch042 branch"
git merge-base --is-ancestor "$EXPECTED_BASE" HEAD || fail "Patch042 base is not an ancestor of HEAD"
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail "Patch042 merge base mismatch"
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail "merge commits are forbidden inside Patch042 source branch"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main must remain at verified Patch041A merge"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main must remain at verified Patch041A merge"
test -z "$(git diff --cached --name-only)" || fail "staged paths are forbidden in Patch042 postcommit validation"
test -z "$(git diff --name-only)" || fail "unstaged tracked paths are forbidden in Patch042 postcommit validation"
test -z "$(git ls-files --others --exclude-standard)" || fail "untracked paths are forbidden in Patch042 postcommit validation"

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig)' "$ACTUAL"; then
  fail "firmware, managed-component, config or sdkconfig scope detected"
fi

NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required for Patch042 validation; observed major=$NODE_MAJOR"

node - <<'NODE' || fail "pinned Homey API dependency identity changed"
const p = require('./tools/homey-inventory/package.json');
const lock = require('./tools/homey-inventory/package-lock.json');
const homey = lock.packages?.['node_modules/homey-api'];
if (p.dependencies?.['homey-api'] !== '3.19.1') process.exit(2);
if (homey?.version !== '3.19.1') process.exit(3);
if (homey?.integrity !== 'sha512-56sd8LvC/CBCOqYwph+FEg0TUY0gkjWJ33+JDlOhAtVNRiZtByj4CxGrdYYFtUx70ufs1eASs1ad3rQD7PWhKA==') process.exit(4);
NODE

RUNNER=scripts/run_patch_042_read_only_awning_capture.sh
LIVE=tools/homey-inventory/src/awning-live-capture.ts
SELECTION=tools/homey-inventory/src/awning-private-selection.ts
LIVE_TEST=tools/homey-inventory/test/awning-live-capture.test.ts
SELECTION_TEST=tools/homey-inventory/test/awning-private-selection.test.ts

bash -n "$RUNNER"
bash -n scripts/validate_patch_042.sh
require_literal 'Node 24 or newer is required' "$RUNNER"
require_literal 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED' "$RUNNER"
require_literal 'PATCH042_FLOW_EXECUTION=NOT_RUN' "$RUNNER"
require_literal 'PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN' "$RUNNER"

require_literal 'MacOSKeychainCredentialProvider' "$LIVE"
require_literal 'createPatch039LocalReadonlyClient' "$LIVE"
require_literal 'collectPatch039ReadSurface' "$LIVE"
require_literal 'publishAwningEvidence' "$LIVE"
require_literal 'client.getDevices()' "$LIVE"
require_literal 'identity_gate_before_credential: true' "$LIVE"
require_literal 'homey_mutation: "forbidden"' "$LIVE"
require_literal 'flow_execution: "forbidden"' "$LIVE"
require_literal 'advanced_flow_execution: "forbidden"' "$LIVE"
require_literal 'Patch042 candidate discovery exceeded the devices-only read boundary' "$LIVE"
require_literal 'awning_selection.json' "$LIVE"
require_literal 'awning_evidence.json' "$LIVE"
require_literal 'resolve(import.meta.dirname, "../../../..")' "$LIVE"

require_literal 'canonical device alias' "$SELECTION"
require_literal 'three unique devices' "$SELECTION"
require_literal 'outside the repository' "$SELECTION"
require_literal 'permissions must not allow group or other access' "$SELECTION"

if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$LIVE" "$SELECTION"; then
  fail "Homey mutation, Flow execution or generic command call found in Patch042 source"
fi
if grep -nE 'HomeyAPI\.createLocalAPI[[:space:]]*\(' "$LIVE" "$SELECTION"; then
  fail "unbounded HomeyAPI.createLocalAPI use found in Patch042 source"
fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$LIVE" "$SELECTION"; then
  fail "direct network primitive found in Patch042 source"
fi
if grep -nE 'EnvironmentCredentialProvider|process\.env.*token|Bearer[[:space:]]+[A-Za-z0-9._~+/-]{8,}' "$LIVE" "$SELECTION"; then
  fail "forbidden environment credential or literal bearer material found"
fi

require_literal 'candidate discovery performs exactly one devices read' "$LIVE_TEST"
require_literal 'performs no capability fallback or execution' "$LIVE_TEST"
require_literal 'publishes sanitized evidence' "$LIVE_TEST"
require_literal 'private selection loader requires outside-repository 0600 regular file' "$SELECTION_TEST"

require_literal 'ACTIVE_FUNCTIONAL_DEVELOPMENT_PATCH=PATCH042' docs/handoff/CURRENT_STATE.md
require_literal 'PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN' docs/handoff/CURRENT_STATE.md
require_literal 'Patch042 - Live Strict Local Read-Only Awning Evidence Capture Runner' docs/history/PATCH_HISTORY.md
require_literal 'PATCH_042_LIVE_STRICT_LOCAL_READ_ONLY_AWNING_EVIDENCE_CAPTURE_RUNNER.md' docs/handoff/MASTER_INDEX.md

git diff --check "$EXPECTED_BASE" --

(
  cd tools/homey-inventory
  npm test
)

printf '%s\n' 'PATCH042_OFFLINE_VALIDATION=PASS'
printf '%s\n' 'PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN'
printf '%s\n' 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PATCH042_FLOW_EXECUTION=NOT_RUN'
printf '%s\n' 'PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN'
printf '%s\n' 'PASS: Patch042 live strict-local read-only awning capture runner offline validation'
