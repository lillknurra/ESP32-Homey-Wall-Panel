#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

EXPECTED_BRANCH="patch-005-live-homey-inventory-validation"
EXPECTED_BASE="bdf75140e8142d926cff090317423607d5463543"

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail "unexpected branch"
test "$(git merge-base HEAD main)" = "$EXPECTED_BASE" || fail "unexpected patch base"
git diff --check

# Exact Patch 005 transport scope. This replaces the historical broad
# `components/` ban, which is incompatible with the now-approved native
# component work.
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/patch005-transport-validator.XXXXXX")"
trap 'rm -rf "$BUILD_DIR"' EXIT

cat > "$BUILD_DIR/expected-scope.txt" <<'SCOPE'
components/athom_cloud_native/CMakeLists.txt
components/athom_cloud_native/include/athom_endpoints.h
components/athom_cloud_native/include/athom_http_esp.h
components/athom_cloud_native/include/athom_provisioning_portal.h
components/athom_cloud_native/src/athom_endpoints.c
components/athom_cloud_native/src/athom_http_esp.c
components/athom_cloud_native/src/athom_provisioning_portal.c
components/athom_cloud_native/test_host/test_transport_contract.c
docs/architecture/ATHOM_ENDPOINT_CAPABILITY_MATRIX.md
docs/handoff/CURRENT_STATE.md
docs/handoff/HANDOFF.md
docs/history/PATCH_005_ATHOM_TRANSPORT_PROVISIONING.md
scripts/validate_patch_005_athom_transport.sh
SCOPE

# The central manifest owns the complete Patch 005 source scope.
# This validator owns only the required transport subset and must not reject
# later Patch 005 implementation files that are valid members of that manifest.
test -f config/patch_005_scope_manifest.txt ||
  fail "central Patch 005 scope manifest missing"

sort -u "$BUILD_DIR/expected-scope.txt" -o "$BUILD_DIR/expected-scope.txt"
sort -u config/patch_005_scope_manifest.txt > "$BUILD_DIR/actual-scope.txt"

comm -23   "$BUILD_DIR/expected-scope.txt"   "$BUILD_DIR/actual-scope.txt"   > "$BUILD_DIR/missing-transport-scope.txt"

if test -s "$BUILD_DIR/missing-transport-scope.txt"; then
  cat "$BUILD_DIR/missing-transport-scope.txt"
  fail "required transport implementation scope missing from central manifest"
fi

printf 'PASS: required transport implementation scope is present in central manifest\n'

# Preserve the original Patch 005 offline foundation controls.
grep -q -- '"homey-api": "3.19.1"' tools/homey-inventory/package.json ||
  fail "homey-api pin changed"
grep -q -- 'explicit --live' tools/homey-inventory/src/homey-api-adapter.ts ||
  fail "live gate missing"
grep -q -- 'ALLOWED_READ_OPERATIONS' tools/homey-inventory/src/call-ledger.ts ||
  fail "read allowlist missing"
grep -q -- 'outside the repository' tools/homey-inventory/src/private-config.ts ||
  fail "private config boundary missing"

if grep -R -n -E 'setCapabilityValue\(|triggerFlow\(|startFlow\(|activateMood\(|genericApiCall\(' \
  tools/homey-inventory/src --exclude=readonly-client.ts; then
  fail "forbidden mutation call found"
fi

grep -q -- 'PATCH_005_LIVE_AUTHORIZED' scripts/run_patch_005_private_live_validation.sh ||
  fail "guarded live runner missing"

if grep -R -n -E 'console\.(log|error).*token|password.*console\.(log|error)' \
  tools/homey-inventory/src; then
  fail "possible credential logging found"
fi

npm --prefix tools/homey-inventory ci
npm --prefix tools/homey-inventory test
bash scripts/validate_patch_004.sh
printf 'PASS: Patch 005 offline foundation validation\n'

# Preserve Controlled Live Preflight hardening checks.
grep -q -- 'repositoryRoot: string' tools/homey-inventory/src/preflight.ts ||
  fail "preflight repository root missing"
grep -q -- 'realpath(resolve(repositoryRoot))' tools/homey-inventory/src/preflight.ts ||
  fail "preflight canonical repository path missing"
grep -q -- 'Private config must be outside the repository' tools/homey-inventory/src/preflight.ts ||
  fail "preflight repository boundary missing"
grep -q -- 'selected_candidate: CandidateKind | null' tools/homey-inventory/src/live-evidence.ts ||
  fail "strict CandidateKind evidence missing"
grep -q -- 'ALLOWED_READ_OPERATIONS.includes(operation)' tools/homey-inventory/src/live-evidence.ts ||
  fail "evidence operation allowlist validation missing"

test -f scripts/setup_patch_005_private_config.sh ||
  fail "missing private config setup script"
test -f scripts/run_patch_005_controlled_preflight.sh ||
  fail "missing controlled preflight runner"
test -f tools/homey-inventory/src/preflight-evidence.ts ||
  fail "missing sanitized preflight evidence"
test -f tools/homey-inventory/src/preflight-runner.ts ||
  fail "missing preflight-only runner"

grep -q 'find-generic-password", "-s"' tools/homey-inventory/src/credential-provider.ts ||
  fail "missing Keychain existence check"

if grep -n 'process\.env\[' \
  tools/homey-inventory/src/preflight-runner.ts \
  tools/homey-inventory/src/credential-provider.ts; then
  fail "controlled preflight must not read environment credential values"
fi

grep -q 'Environment provider is refused during controlled preflight' \
  tools/homey-inventory/src/credential-provider.ts ||
  fail "environment provider is not explicitly refused"
grep -q 'JSON.stringify(value,null,2)' scripts/setup_patch_005_private_config.sh ||
  fail "private config setup does not use JSON-safe serialization"
grep -q 'JSON.parse(fs.readFileSync' scripts/setup_patch_005_private_config.sh ||
  fail "generated private config is not validated"

if grep -nE 'homey-api|HomeyAPI|fetch\(|https?://|WebSocket|net\.|dns\.' \
  tools/homey-inventory/src/preflight-runner.ts; then
  fail "network or Homey client boundary detected in preflight runner"
fi

grep -q 'credential_value_read: false' tools/homey-inventory/src/preflight-evidence.ts ||
  fail "credential read flag is not hard false"
grep -q 'network_access_attempted: false' tools/homey-inventory/src/preflight-evidence.ts ||
  fail "network flag is not hard false"

bash -n scripts/setup_patch_005_private_config.sh
bash -n scripts/run_patch_005_controlled_preflight.sh
printf 'PASS: Controlled Live Preflight static boundary\n'

# New native transport checks.
cc -std=c11 -Wall -Wextra -Werror \
  -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_transport_contract.c \
  -o "$BUILD_DIR/test_transport_contract"
"$BUILD_DIR/test_transport_contract"

cc -std=c11 -Wall -Wextra -Werror \
  -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_auth.c \
  components/athom_cloud_native/src/athom_homey_client.c \
  components/athom_cloud_native/src/athom_provisioning.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_athom_cloud_native.c \
  -o "$BUILD_DIR/test_athom_cloud_native"
"$BUILD_DIR/test_athom_cloud_native"

grep -q 'esp_crt_bundle_attach' components/athom_cloud_native/src/athom_http_esp.c
grep -q 'ATHOM_HTTP_MAX_RESPONSE_BYTES' components/athom_cloud_native/include/athom_http_esp.h
grep -q 'explicit_local_authorization_required' \
  components/athom_cloud_native/src/athom_provisioning_portal.c

if grep -R -n -E 'oauth/device|code_challenge|code_verifier' \
  components/athom_cloud_native/src; then
  fail "unsupported OAuth grant implemented"
fi

git diff --check
printf 'PASS: Patch 005 Athom transport exact-scope validation\n'
