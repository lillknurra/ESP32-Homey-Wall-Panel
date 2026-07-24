#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat PAGER=cat LESS=FRX
ROOT="$(git rev-parse --show-toplevel)"; cd "$ROOT"
EXPECTED_BRANCH="patch-005-live-homey-inventory-validation"
EXPECTED_HEAD="7db4cf2cc58fb8554979d448f25a9c65ac818169"
EXPECTED_MAIN="bdf75140e8142d926cff090317423607d5463543"
MANIFEST="config/patch_005_scope_manifest.txt"
fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail branch
test "$(git rev-parse HEAD)" = "$EXPECTED_HEAD" || fail HEAD
test "$(git rev-parse origin/$EXPECTED_BRANCH)" = "$EXPECTED_HEAD" || fail origin-head
test "$(git rev-parse main)" = "$EXPECTED_MAIN" || fail main
test "$(git rev-parse origin/main)" = "$EXPECTED_MAIN" || fail origin-main
test -f "$MANIFEST" || fail scope-manifest
TMP="$(mktemp -d "${TMPDIR:-/tmp}/p5-final.XXXXXX")"; trap 'rm -rf "$TMP"' EXIT
sed '/^[[:space:]]*$/d' "$MANIFEST" | sort -u > "$TMP/expected"
{ git diff --name-only; git ls-files --others --exclude-standard; } | sed '/^[[:space:]]*$/d' | sort -u > "$TMP/actual"
diff -u "$TMP/expected" "$TMP/actual" || fail exact-scope
git diff --check

# Existing TypeScript and Patch 004 baseline.
grep -q '"homey-api": "3.19.1"' tools/homey-inventory/package.json || fail homey-api-pin
npm --prefix tools/homey-inventory ci
npm --prefix tools/homey-inventory test
bash scripts/validate_patch_004.sh

# Native suites. Every binary is linked with its full dependency set.
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_transport_contract.c \
  -o "$TMP/transport-contract"
"$TMP/transport-contract"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_auth.c \
  components/athom_cloud_native/src/athom_homey_client.c \
  components/athom_cloud_native/src/athom_provisioning.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_athom_cloud_native.c \
  -o "$TMP/native-foundation"
"$TMP/native-foundation"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_runtime.c \
  components/athom_cloud_native/test_host/test_executable_protocol.c \
  -o "$TMP/protocol"
"$TMP/protocol"
cc -std=c11 -Wall -Wextra -Werror -Icomponents/athom_cloud_native/include \
  components/athom_cloud_native/src/athom_endpoints.c \
  components/athom_cloud_native/src/athom_http_esp.c \
  components/athom_cloud_native/src/athom_protocol.c \
  components/athom_cloud_native/src/athom_redaction.c \
  components/athom_cloud_native/test_host/test_live_transport.c \
  -o "$TMP/live-transport"
"$TMP/live-transport"

# Live-readiness gates.
! grep -n 'not_ready_' components/athom_cloud_native/src/athom_http_esp.c || fail active-not-ready
! grep -n 'ATHOM_ERR_UNSUPPORTED' components/athom_cloud_native/src/athom_http_esp.c || fail active-unsupported
awk '/basic_header\(/ {n++} END {exit !(n>1)}' components/athom_cloud_native/src/athom_http_esp.c || fail basic-header-unused
awk '/execute\(/ {n++} END {exit !(n>3)}' components/athom_cloud_native/src/athom_http_esp.c || fail request-unused
grep -q 'exchange_authorization_code = exchange_authorization_code' components/athom_cloud_native/src/athom_http_esp.c || fail exchange-vtable
grep -q 'refresh_access_token = refresh_access_token' components/athom_cloud_native/src/athom_http_esp.c || fail refresh-vtable
grep -q 'list_homeys = list_homeys' components/athom_cloud_native/src/athom_http_esp.c || fail list-vtable
grep -q 'read_inventory = read_inventory' components/athom_cloud_native/src/athom_http_esp.c || fail inventory-vtable
grep -q 'esp_crt_bundle_attach' components/athom_cloud_native/src/athom_http_esp.c || fail tls-bundle
grep -q 'ATHOM_HTTP_MAX_RESPONSE_BYTES' components/athom_cloud_native/include/athom_http_esp.h || fail response-limit
grep -q 'IP_EVENT_STA_GOT_IP' components/athom_cloud_native/src/athom_runtime.c || fail got-ip
# Patch 005H.1 owns the active firmware entrypoint. Athom runtime and OAuth
# remain compiled and host-tested but must not be activated before a separately
# approved live-authentication patch.
grep -q 'secure_bootstrap' main/main.c || fail secure-bootstrap-wiring
grep -q 'oauth_locked=true' components/secure_bootstrap/secure_bootstrap_esp.c ||
  fail oauth-lock-marker

if grep -q 'athom_runtime_start' main/main.c; then
  fail unexpected-active-athom-runtime
fi

if grep -q 'athom_provisioning_portal_start' main/main.c; then
  fail unexpected-active-athom-provisioning-portal
fi
for uri in /live/authorize /configure /oauth/callback /homeys /homeys/select /inventory/run /credentials/wipe; do
  grep -q "$uri" components/athom_cloud_native/src/athom_provisioning_portal.c || fail "handler-$uri"
done
! grep -R -n -E 'setCapabilityValue|triggerFlow|startFlow|activateMood|genericApiCall' components/athom_cloud_native/src components/athom_cloud_native/include || fail mutation-surface
! grep -R -n -E 'oauth/device|code_challenge|code_verifier' components/athom_cloud_native/src || fail undocumented-grant
python3 - <<'PY'
from pathlib import Path
src=Path('scripts/run_patch_005_hardware_live.py').read_text(encoding='utf-8')
compile(src,'hardware-runner','exec')
assert 'start_new_session=True' in src
assert 'timeout ' not in src
assert "Type FLASH" in src and "Type OAUTH" in src
PY
git diff --check
printf 'PASS: Patch 005 final executable native offline validation\n'
