#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="72c045dce5366e933309200f5f9287581d8252b6"
EXPECTED_BRANCH="patch-039-host-only-read-only-awning-evidence-collector"
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
  scripts/run_patch_039_read_only_capture.sh \
  scripts/validate_patch_039.sh \
  tools/homey-inventory/src/aliases.ts \
  tools/homey-inventory/src/awning-call-ledger.ts \
  tools/homey-inventory/src/awning-candidate-classifier.ts \
  tools/homey-inventory/src/awning-evidence-builder.ts \
  tools/homey-inventory/src/awning-evidence-normalizer.ts \
  tools/homey-inventory/src/awning-evidence-publication.ts \
  tools/homey-inventory/src/awning-homey-api-adapter.ts \
  tools/homey-inventory/src/awning-model.ts \
  tools/homey-inventory/src/awning-operator-selection.ts \
  tools/homey-inventory/src/awning-private-config.ts \
  tools/homey-inventory/src/awning-private-mapping.ts \
  tools/homey-inventory/src/awning-readonly-client.ts \
  tools/homey-inventory/test/awning-classifier.test.ts \
  tools/homey-inventory/test/awning-identity-readonly.test.ts \
  tools/homey-inventory/test/awning-private-config.test.ts \
  tools/homey-inventory/test/awning-private-mapping.test.ts \
  tools/homey-inventory/test/awning-publication.test.ts | sort > "$EXPECTED"

{
  git diff --name-only "$EXPECTED_BASE" --
  git ls-files --others --exclude-standard
} | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected branch"
test "$(git rev-parse HEAD)" = "$EXPECTED_BASE" || fail "Patch039 must remain uncommitted at the stable base"
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail "local main moved during Patch039 offline implementation"
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail "origin/main moved during Patch039 offline implementation"
test -z "$(git diff --cached --name-only)" || fail "staged files are forbidden in offline foundation gate"
diff -u "$EXPECTED" "$ACTUAL" || fail "Patch039 exact 19-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "19" || fail "Patch039 file count is not 19"

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig|docs/)' "$ACTUAL"; then
  fail "firmware, config, sdkconfig, managed-component, or documentation scope detected"
fi

node -e 'const p=require("./tools/homey-inventory/package.json"); if(p.dependencies?.["homey-api"]!=="3.19.1") process.exit(2);' || fail "homey-api package pin changed"
node -e 'const p=require("./tools/homey-inventory/package-lock.json"); const h=p.packages?.["node_modules/homey-api"]; if(h?.version!=="3.19.1" || h?.integrity!=="sha512-56sd8LvC/CBCOqYwph+FEg0TUY0gkjWJ33+JDlOhAtVNRiZtByj4CxGrdYYFtUx70ufs1eASs1ad3rQD7PWhKA==") process.exit(2);' || fail "homey-api 3.19.1 lock/SRI changed"

ADAPTER=tools/homey-inventory/src/awning-homey-api-adapter.ts
require_literal 'package_version: "3.19.1"' "$ADAPTER"
require_literal 'identity_header: "X-Homey-ID"' "$ADAPTER"
require_literal 'credential_kind: "personal_access_token"' "$ADAPTER"
require_literal 'candidate_fallback: false' "$ADAPTER"
require_literal 'live_factory_wired: false' "$ADAPTER"
require_literal 'verifyPatch039SelectedHomey(rawApi.id, expectedHomeyDigest)' "$ADAPTER"
require_literal 'capability-value fallback requires a successful fresh getDevices read' "$ADAPTER"
require_literal 'capability-value fallback pair was not observed as missing current-value evidence' "$ADAPTER"
require_literal 'conditionalCapabilityValueFallbacks.delete(key)' "$ADAPTER"
if grep -nE 'HomeyAPI\.createLocalAPI[[:space:]]*\(' "$ADAPTER"; then
  fail "offline foundation must not wire the live Homey factory"
fi

PRIVATE_CONFIG=tools/homey-inventory/src/awning-private-config.ts
require_literal 'connection: "local"' "$PRIVATE_CONFIG"
require_literal 'credential_kind: "personal_access_token"' "$PRIVATE_CONFIG"
require_literal 'credential_provider: "macos-keychain"' "$PRIVATE_CONFIG"
require_literal 'expected_homey_id_sha256' "$PRIVATE_CONFIG"
require_literal 'must be outside the repository' "$PRIVATE_CONFIG"
require_literal 'permissions must not allow group or other access' "$PRIVATE_CONFIG"
require_literal 'secure target downgrade is forbidden' "$PRIVATE_CONFIG"
require_literal 'redirect or target substitution is forbidden' "$PRIVATE_CONFIG"

MAPPING=tools/homey-inventory/src/awning-private-mapping.ts
require_literal 'purpose: "read_only_awning_evidence"' "$MAPPING"
require_literal 'awning_1' tools/homey-inventory/src/awning-model.ts
require_literal 'awning_2' tools/homey-inventory/src/awning-model.ts
require_literal 'awning_3' tools/homey-inventory/src/awning-model.ts
require_literal 'Private awning mapping devices must be unique' "$MAPPING"
require_literal 'stale or ambiguous' "$MAPPING"
require_literal 'selected-Homey mismatch' "$MAPPING"

ALIASES=tools/homey-inventory/src/aliases.ts
for kind in driver capability flow_card advanced_flow_card owner; do
  grep -Fq "$kind" "$ALIASES" || fail "missing Patch039 alias kind: $kind"
done

NORMALIZER=tools/homey-inventory/src/awning-evidence-normalizer.ts
for marker in \
  CAPABILITY_LIST_INCONSISTENT \
  CAPABILITY_OPTIONS_UNINSPECTED \
  FLOW_ARGS_UNVERIFIED \
  FLOW_ARGS_TARGET_NOT_EXACT \
  ADVANCED_FLOW_ARGS_UNVERIFIED \
  ADVANCED_FLOW_ARGS_TARGET_NOT_EXACT; do
  require_literal "$marker" "$NORMALIZER"
done

READONLY=tools/homey-inventory/src/awning-readonly-client.ts
for method in \
  'ManagerDevices.getDevices' \
  'ManagerDevices.getCapabilityValue' \
  'ManagerFlow.getFlows' \
  'ManagerFlow.getFlowCardActions' \
  'ManagerFlow.getAdvancedFlows'; do
  grep -Fq "$method" "$READONLY" || fail "missing read allowlist method: $method"
done
for op in devices.read capability_value.read flows.read flow_card_actions.read advanced_flows.read; do
  grep -Fq "$op" tools/homey-inventory/src/awning-call-ledger.ts || fail "missing ledger operation: $op"
done

for classification in \
  NO_CANDIDATE \
  CANDIDATE_DEVICE_CAPABILITY \
  CANDIDATE_FLOW \
  CANDIDATE_ADVANCED_FLOW \
  AMBIGUOUS \
  INSUFFICIENT_EVIDENCE; do
  grep -Fq "$classification" tools/homey-inventory/src/awning-model.ts || fail "missing classification: $classification"
done
if grep -R -n -E 'VERIFIED_COMMAND|ACTIONABLE|EXECUTION_READY|PRIVATE_ACTION_AUTHORIZED' \
  tools/homey-inventory/src/awning-*.ts tools/homey-inventory/test/awning-*.test.ts; then
  fail "forbidden stronger command-state vocabulary found"
fi

if grep -R -n -E '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' \
  tools/homey-inventory/src/awning-*.ts; then
  fail "forbidden Homey mutation/execution call found in Patch039 source"
fi
if grep -R -n -E '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' tools/homey-inventory/src/awning-*.ts; then
  fail "direct network primitive found in Patch039 offline foundation"
fi

PUBLICATION=tools/homey-inventory/src/awning-evidence-publication.ts
require_literal 'Patch039 output must remain outside the repository' "$PUBLICATION"
require_literal 'Raw Homey identifier leaked into Patch039 evidence' "$PUBLICATION"
require_literal 'Sensitive value pattern detected in Patch039 evidence' "$PUBLICATION"
require_literal 'publication_attempted: true' "$PUBLICATION"
require_literal 'assertDeviceEvidence' "$PUBLICATION"
require_literal 'assertFlowEvidence' "$PUBLICATION"
require_literal 'assertAdvancedFlowEvidence' "$PUBLICATION"
require_literal 'failure_class is outside the bounded failure domain' "$PUBLICATION"

CLASSIFIER_TEST=tools/homey-inventory/test/awning-classifier.test.ts
require_literal 'capabilities list missing capabilitiesObj evidence blocks NO_CANDIDATE' "$CLASSIFIER_TEST"
require_literal 'non-empty capability options block device-capability candidate promotion' "$CLASSIFIER_TEST"
require_literal 'nested or additional saved Flow args block descriptor-only promotion' "$CLASSIFIER_TEST"
require_literal 'nested or additional Advanced Flow args block descriptor-only promotion' "$CLASSIFIER_TEST"

IDENTITY_TEST=tools/homey-inventory/test/awning-identity-readonly.test.ts
require_literal 'conditional capability-value fallback refuses calls before fresh getDevices' "$IDENTITY_TEST"
require_literal 'conditional capability-value fallback refuses unobserved pair and already-present current value before Homey call' "$IDENTITY_TEST"
require_literal 'conditional capability-value fallback allows exactly one observed missing-value pair per fresh device inventory' "$IDENTITY_TEST"

PUBLICATION_TEST=tools/homey-inventory/test/awning-publication.test.ts
require_literal 'extra nested private field is rejected and previous accepted evidence is preserved' "$PUBLICATION_TEST"
require_literal 'invalid nested aliases and ledger failure classes are rejected before replacement' "$PUBLICATION_TEST"

RUNNER=scripts/run_patch_039_read_only_capture.sh
bash -n "$RUNNER"
require_literal 'PATCH039_REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN' "$RUNNER"
require_literal 'PATCH039_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED' "$RUNNER"
require_literal 'does not wire a live Homey factory or read credentials' "$RUNNER"

if grep -R -n -E 'https?://|/Users/[^/<[:space:]]+|Bearer[[:space:]]+[A-Za-z0-9._~+/-]{8,}' \
  tools/homey-inventory/src/awning-*.ts tools/homey-inventory/test/awning-*.test.ts scripts/run_patch_039_read_only_capture.sh; then
  fail "literal URL, macOS user path, or credential-like bearer value found"
fi

bash -n scripts/validate_patch_039.sh
git diff --check

printf '%s\n' 'STATIC_SOURCE_PASS'
printf '%s\n' 'REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN'
printf '%s\n' 'HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch039 corrected offline source boundary validation'
