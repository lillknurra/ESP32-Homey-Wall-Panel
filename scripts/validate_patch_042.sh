#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="85ff9d0a5da95ac086ae396c6cf16ce0b02961f3"
EXPECTED_BRANCH="patch-042-live-read-only-awning-evidence-capture-enablement"
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

fail() { printf 'FAIL: %s\n' "$*" >&2; exit 1; }
require_literal() { grep -Fq -- "$1" "$2" || fail "missing required literal in $2: $1"; }

EXPECTED="$(mktemp)"
ACTUAL="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL"' EXIT

printf '%s\n' \
  scripts/run_patch_042_live_read_only_awning_capture.sh \
  scripts/validate_patch_042.sh \
  tools/homey-inventory/src/awning-live-capture-cli.ts \
  tools/homey-inventory/src/awning-live-capture.ts \
  tools/homey-inventory/test/awning-live-capture.test.ts | sort > "$EXPECTED"

{
  git diff --name-only "$EXPECTED_BASE" --
  git ls-files --others --exclude-standard
} | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail 'unexpected branch'
test "$(git rev-parse "$EXPECTED_BASE")" = "$EXPECTED_BASE" || fail 'Patch042 base missing'
test -z "$(git diff --cached --name-only)" || fail 'staged files are forbidden before Patch042 validation'
diff -u "$EXPECTED" "$ACTUAL" || fail 'Patch042 exact five-file scope mismatch'
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = '5' || fail 'Patch042 file count is not five'

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig|docs/)' "$ACTUAL"; then
  fail 'firmware, config, sdkconfig, managed-component, or documentation scope detected'
fi

node -e 'const p=require("./tools/homey-inventory/package.json"); if(p.dependencies?.["homey-api"]!=="3.19.1") process.exit(2); if(Number(process.versions.node.split(".")[0])<24) process.exit(3);' \
  || fail 'Node >=24 or pinned homey-api 3.19.1 gate failed'

SRC=tools/homey-inventory/src/awning-live-capture.ts
CLI=tools/homey-inventory/src/awning-live-capture-cli.ts
RUNNER=scripts/run_patch_042_live_read_only_awning_capture.sh
TEST=tools/homey-inventory/test/awning-live-capture.test.ts

require_literal 'transport: "patch040_strict_local_get_only"' "$SRC"
require_literal 'homey_mutation: false' "$SRC"
require_literal 'flow_execution: false' "$SRC"
require_literal 'advanced_flow_execution: false' "$SRC"
require_literal 'write_methods: [] as const' "$SRC"
require_literal 'createPatch039LocalReadonlyClient' "$SRC"
require_literal 'collectPatch039ReadSurface' "$SRC"
require_literal 'buildPrivateOperatorCandidates' "$SRC"
require_literal 'createPrivateAwningMappingFromSelection' "$SRC"
require_literal 'buildAwningEvidence' "$SRC"
require_literal 'publishAwningEvidence' "$SRC"
require_literal 'MacOSKeychainCredentialProvider' "$SRC"
require_literal 'loadPatch039PrivateConfig' "$SRC"
require_literal 'loadPrivateAwningMapping' "$SRC"
require_literal 'assertMappingMatchesSelectedHomey' "$SRC"
require_literal 'savePrivateAwningMapping' "$SRC"

for forbidden in setCapabilityValue runFlowCardAction triggerFlow testFlow triggerAdvancedFlow activateMood createFlow updateFlow deleteFlow createAdvancedFlow updateAdvancedFlow deleteAdvancedFlow genericAapiCall; do
  if grep -R -n -F ".$forbidden("z "$SRC" "$CMI"; then
    fail "forbidden Homey mutation/execution call found: $forbidden"
  fi
done
if grep -R -n -E '\b(fetch|axios|https?\.request|http\/.request)[[:space:]]*\(' "$SRC" "$CLI"; then
  fail 'direct network primitive found in Patch042 orchestration'
fi
if grep -R -n -E 'process\.env.*(TOKEN|PAT|BEARER)|Authorization[[:space:]]*=' "$SRC" "$CLI"; then
  fail 'Patch042 must not read credentials from environment or construct auth headers'
fi

require_literal 'First argument must be inspect, map, or capture' "$CLI"
require_literal '--private-config must be an absolute path' "$CLI"
require_literal '--mapping must be an absolute path' "$CLI"
require_literal '--output must be an absolute path' "$CLI"
require_literal 'console.error(`FAIL: ${redact(message)}`)' "$CLI"

bash -n "$RUNNER"
require_literal 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED' "$RUNNER"
require_literal 'PATCH042_FLOW_EXECUTION=NOT_RUN' "$RUNNER"
require_literal 'PATCH042_ADVANCED_FLOW_EXECUTION=NOT_RUN' "$RUNNER"
require_literal 'Node.js >=24 is required' "$RUNNER"

require_literal 'inspect publishes only sanitized candidates and no raw device IDs' "$TEST"
require_literal 'persists raw correlation only in private mapping' "$TEST"
require_literal 'capture rejects selected-Homey mapping mismatch before client construction' "$TEST"
require_literal 'capture executes only the locked base read surface and publishes sanitized evidence' "$TEST"

npm --prefix tools/homey-inventory test

# Patch040 strict transport remains untouched by the exact Patch042 scope.
require_literal 'mutation_surface: "none"' tools/homey-inventory/src/patch039-strict-local-transport.ts
require_literal 'create_local_api_used: false' tools/homey-inventory/src/awning-homey-api-adapter.ts
require_literal 'credential_retrieval: "lazy_after_identity_gate"' tools/homey-inventory/src/awning-homey-api-adapter.ts
require_literal 'ManagerDevices.getDevices' tools/homey-inventory/src/awning-readonly-client.ts
require_literal 'ManagerFlow.getAdvancedFlows' tools/homey-inventory/src/awning-readonly-client.ts
git diff --check

printf '%s\n' 'PATCH042_STATIC_SOURCE=PASS'
printf '%s\n' 'PATCH042_HOST_TESTS=PASS'
printf '%s\n' 'PATCH040_STRICT_TRANSPORT_INVARIANTS=PASS'
printf '%s\n' 'PATCH042_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN'
printf '%s\n' 'PATCH042_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch042 live read-only awning evidence capture enablement validation'
