#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BRANCH="patch-005-live-homey-inventory-validation"
EXPECTED_BASE="bdf75140e8142d926cff090317423607d5463543"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected branch"
test "$(git merge-base HEAD main)" = "$EXPECTED_BASE" || fail "unexpected patch base"
git diff --check

test -z "$(git diff --name-only -- main components config/panel_binding.yaml schemas/homey_inventory.schema.json)" ||
  fail "forbidden firmware, binding, or schema scope"

grep -q -- '"homey-api": "3.19.1"' tools/homey-inventory/package.json ||
  fail "homey-api pin changed"
grep -q -- 'explicit --live' tools/homey-inventory/src/homey-api-adapter.ts ||
  fail "live gate missing"
grep -q -- 'ALLOWED_READ_OPERATIONS' tools/homey-inventory/src/call-ledger.ts ||
  fail "read allowlist missing"
grep -q -- 'outside the repository' tools/homey-inventory/src/private-config.ts ||
  fail "private config boundary missing"

if grep -R -n -E 'setCapabilityValue\(|triggerFlow\(|startFlow\(|activateMood\(|genericApiCall\('   tools/homey-inventory/src --exclude=readonly-client.ts; then
  fail "forbidden mutation call found"
fi

grep -q -- 'PATCH_005_LIVE_AUTHORIZED' scripts/run_patch_005_private_live_validation.sh || fail "guarded live runner missing"
if grep -R -n -E 'console\.(log|error).*token|password.*console\.(log|error)' tools/homey-inventory/src; then fail "possible credential logging found"; fi

npm --prefix tools/homey-inventory ci
npm --prefix tools/homey-inventory test
bash scripts/validate_patch_004.sh

printf 'PASS: Patch 005 offline foundation validation\n'

# PATCH_005_PREFLIGHT_AND_EVIDENCE_HARDENING
grep -q -- 'repositoryRoot: string' tools/homey-inventory/src/preflight.ts || fail "preflight repository root missing"
grep -q -- 'realpath(resolve(repositoryRoot))' tools/homey-inventory/src/preflight.ts || fail "preflight canonical repository path missing"
grep -q -- 'Private config must be outside the repository' tools/homey-inventory/src/preflight.ts || fail "preflight repository boundary missing"
grep -q -- 'selected_candidate: CandidateKind | null' tools/homey-inventory/src/live-evidence.ts || fail "strict CandidateKind evidence missing"
grep -q -- 'ALLOWED_READ_OPERATIONS.includes(operation)' tools/homey-inventory/src/live-evidence.ts || fail "evidence operation allowlist validation missing"
# Controlled Live Preflight static boundary
test -f scripts/setup_patch_005_private_config.sh || fail "missing private config setup script"
test -f scripts/run_patch_005_controlled_preflight.sh || fail "missing controlled preflight runner"
test -f tools/homey-inventory/src/preflight-evidence.ts || fail "missing sanitized preflight evidence"
test -f tools/homey-inventory/src/preflight-runner.ts || fail "missing preflight-only runner"
grep -q 'find-generic-password", "-s"' tools/homey-inventory/src/credential-provider.ts || fail "missing Keychain existence check"
if grep -n 'process\.env\[' tools/homey-inventory/src/preflight-runner.ts tools/homey-inventory/src/credential-provider.ts; then
  fail "controlled preflight must not read environment credential values"
fi
grep -q 'Environment provider is refused during controlled preflight' tools/homey-inventory/src/credential-provider.ts || fail "environment provider is not explicitly refused"
grep -q 'JSON.stringify(value,null,2)' scripts/setup_patch_005_private_config.sh || fail "private config setup does not use JSON-safe serialization"
grep -q 'JSON.parse(fs.readFileSync' scripts/setup_patch_005_private_config.sh || fail "generated private config is not validated"
if grep -nE 'homey-api|HomeyAPI|fetch\(|https?://|WebSocket|net\.|dns\.' tools/homey-inventory/src/preflight-runner.ts; then
  fail "network or Homey client boundary detected in preflight runner"
fi
grep -q 'credential_value_read: false' tools/homey-inventory/src/preflight-evidence.ts || fail "credential read flag is not hard false"
grep -q 'network_access_attempted: false' tools/homey-inventory/src/preflight-evidence.ts || fail "network flag is not hard false"
bash -n scripts/setup_patch_005_private_config.sh
bash -n scripts/run_patch_005_controlled_preflight.sh
printf 'PASS: Controlled Live Preflight static boundary\n'
