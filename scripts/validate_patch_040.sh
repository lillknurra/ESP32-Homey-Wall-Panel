#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX

EXPECTED_BASE="722cedce02b2c30bcafd1aadb0985bd83224b460"
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
STAGED="$(mktemp)"
trap 'rm -f "$EXPECTED" "$ACTUAL" "$STAGED"' EXIT

VALIDATION_STATE="${PATCH040_VALIDATION_STATE:-unstaged}"
case "$VALIDATION_STATE" in
  unstaged|staged) ;;
  *) fail "PATCH040_VALIDATION_STATE must be unstaged or staged" ;;
esac

printf '%s\n' \
  scripts/validate_patch_040.sh \
  tools/homey-inventory/src/awning-homey-api-adapter.ts \
  tools/homey-inventory/src/patch039-strict-local-transport.ts \
  tools/homey-inventory/test/awning-identity-readonly.test.ts \
  tools/homey-inventory/test/patch039-strict-local-transport.test.ts | sort > "$EXPECTED"

{
  git diff --name-only "$EXPECTED_BASE" --
  git ls-files --others --exclude-standard
} | sed '/^[[:space:]]*$/d' | sort -u > "$ACTUAL"

test "$(git rev-parse HEAD)" = "$EXPECTED_BASE" || fail "Patch040 HEAD must remain at the stable base before commit"
test "$(git rev-parse refs/heads/main)" = "$EXPECTED_BASE" || fail "local main moved during Patch040 offline implementation"
test "$(git rev-parse refs/remotes/origin/main)" = "$EXPECTED_BASE" || fail "origin/main moved during Patch040 offline implementation"

if [ "$VALIDATION_STATE" = "staged" ]; then
  git diff --cached --name-only | sed '/^[[:space:]]*$/d' | sort -u > "$STAGED"
  diff -u "$EXPECTED" "$STAGED" || fail "Patch040 staged scope mismatch"
  test "$(wc -l < "$STAGED" | tr -d ' ')" = "5" || fail "Patch040 staged file count is not five"
  test -z "$(git diff --name-only)" || fail "unstaged tracked changes are forbidden in staged validation mode"
  test -z "$(git ls-files --others --exclude-standard)" || fail "untracked files are forbidden in staged validation mode"
else
  test -z "$(git diff --cached --name-only)" || fail "staged files are forbidden before Patch040 review"
fi

diff -u "$EXPECTED" "$ACTUAL" || fail "Patch040 exact five-file scope mismatch"
test "$(wc -l < "$ACTUAL" | tr -d ' ')" = "5" || fail "Patch040 file count is not five"

if grep -E '^(components/|main/|managed_components/|config/|sdkconfig|docs/)' "$ACTUAL"; then
  fail "firmware, config, sdkconfig, managed-component, or documentation scope detected"
fi

node - <<'NODE' || fail "pinned package identity changed"
const p = require('./tools/homey-inventory/package.json');
const lock = require('./tools/homey-inventory/package-lock.json');
const h = lock.packages?.['node_modules/homey-api'];
const n = lock.packages?.['node_modules/node-fetch'];
if (p.dependencies?.['homey-api'] !== '3.19.1') process.exit(2);
if (h?.version !== '3.19.1') process.exit(3);
if (h?.resolved !== 'https://registry.npmjs.org/homey-api/-/homey-api-3.19.1.tgz') process.exit(4);
if (h?.integrity !== 'sha512-56sd8LvC/CBCOqYwph+FEg0TUY0gkjWJ33+JDlOhAtVNRiZtByj4CxGrdYYFtUx70ufs1eASs1ad3rQD7PWhKA==') process.exit(5);
if (n?.version !== '2.7.0') process.exit(6);
if (n?.integrity !== 'sha512-c4FRfUm/dbcWZ7U+1Wq0AwCyFL+3nt2bEw05wfxSz+DWpWsitgmSgYmy2dQdWyKC1694ELPqMs/YzUSNozLt8A==') process.exit(7);
NODE

STRICT=tools/homey-inventory/src/patch039-strict-local-transport.ts
ADAPTER=tools/homey-inventory/src/awning-homey-api-adapter.ts
IDENTITY_TEST=tools/homey-inventory/test/awning-identity-readonly.test.ts
STRICT_TEST=tools/homey-inventory/test/patch039-strict-local-transport.test.ts

require_literal 'redirect: "manual"' "$STRICT"
require_literal 'new URL(response.url).origin' "$STRICT"
require_literal 'assertPatch039ObservedAddressPolicy(configuredOrigin, observedOrigin)' "$STRICT"
require_literal 'ERR_SOCKET_SESSION_NOT_READY' "$STRICT"
require_literal 'method !== "GET"' "$STRICT"
require_literal 'Patch039 strict transport forbids request bodies' "$STRICT"
require_literal 'caller-supplied credential or target headers are forbidden' "$STRICT"
require_literal 'Patch039 Homey discovery is forbidden' "$STRICT"
require_literal 'Patch039 login and token refresh are forbidden' "$STRICT"
require_literal 'getPersonalAccessToken' "$STRICT"
require_literal 'const personalAccessToken = (await input.getPersonalAccessToken()).trim()' "$STRICT"
require_literal 'baseUrl: configuredOrigin' "$STRICT"
require_literal 'strategy: []' "$STRICT"
require_literal 'reconnect: false' "$STRICT"
require_literal 'api: undefined' "$STRICT"
require_literal 'create_local_api_used: false' "$ADAPTER"
require_literal 'credential_retrieval: "lazy_after_identity_gate"' "$ADAPTER"

if grep -nE '^[[:space:]]*export.*createPatch039StrictHomeySubclass' "$STRICT"; then
  fail "raw strict Homey subclass factory must remain module-private"
fi
if grep -nE '^[[:space:]]*export.*loadPatch039HomeyApiRuntime' "$STRICT"; then
  fail "raw Homey API runtime loader must remain module-private"
fi

if grep -nE 'HomeyAPI\.createLocalAPI[[:space:]]*\(' "$STRICT" "$ADAPTER"; then
  fail "HomeyAPI.createLocalAPI live use is forbidden"
fi

if grep -R -n -E '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' \
  "$STRICT" "$ADAPTER"; then
  fail "Homey mutation or generic execution call found in Patch040 implementation"
fi

for method in \
  'ManagerDevices.getDevices' \
  'ManagerDevices.getCapabilityValue' \
  'ManagerFlow.getFlows' \
  'ManagerFlow.getFlowCardActions' \
  'ManagerFlow.getAdvancedFlows'; do
  grep -Fq "$method" tools/homey-inventory/src/awning-readonly-client.ts || fail "read allowlist changed: $method"
done

for marker in \
  'invalid configured address fails before PAT retrieval and before fetch' \
  'identity ping is unauthenticated and explicitly uses redirect manual' \
  'redirected response flag fails closed before PAT retrieval' \
  'observed response origin mismatch fails before PAT retrieval' \
  'identity mismatch fails before PAT retrieval' \
  'successful identity gate makes exactly one transition to PAT retrieval' \
  'authenticated manager read is GET-only, bearer-authenticated, manual-redirect, and socket-fallback only' \
  'socket __apiRequest fails locally with the exact manager fallback code' \
  'strict transport refuses non-GET, body-bearing, caller auth, target headers, and non-allowlisted manager paths before fetch' \
  'connect login subscribe and discovery are fail-closed' \
  'authenticated redirect is rejected without a second target request'; do
  require_literal "$marker" "$STRICT_TEST"
done
require_literal 'strict subclass factory is not exported from the production module' "$STRICT_TEST"
require_literal 'strict runtime loader is not exported from the production module' "$STRICT_TEST"
require_literal 'for (const status of [301, 302, 303, 307, 308])' "$STRICT_TEST"
require_literal 'test(`redirect ${status} fails closed before PAT retrieval`' "$STRICT_TEST"
require_literal 'conditional capability-value fallback allows exactly one observed missing-value pair per fresh device inventory' "$IDENTITY_TEST"

if grep -R -n -E 'Bearer[ \t]+[A-Za-z0-9._~+/-]{16,}' \
  "$STRICT" "$ADAPTER" "$IDENTITY_TEST" "$STRICT_TEST"; then
  fail "credential-like bearer value found in Patch040 scope"
fi
if grep -R -n -E 'https?://' "$STRICT" "$ADAPTER" "$IDENTITY_TEST" "$STRICT_TEST"; then
  fail "literal network target found in Patch040 scope"
fi
if grep -R -n -E '/Users/[^/<[:space:]]+' "$STRICT" "$ADAPTER" "$IDENTITY_TEST" "$STRICT_TEST"; then
  fail "macOS user path found in Patch040 scope"
fi

(
  cd tools/homey-inventory
  npm run build
  node --test dist/test/*.test.js
)

if [ "$VALIDATION_STATE" = "staged" ]; then
  git diff --cached --check
else
  git diff --check
fi

printf 'PATCH040_VALIDATION_STATE=%s\n' "$(printf '%s' "$VALIDATION_STATE" | tr '[:lower:]' '[:upper:]')"
printf '%s\n' 'PATCH040_SCOPE_GATE=PASS'
printf '%s\n' 'STRICT_PING_AUTH_STATUS=UNAUTHENTICATED_NO_BEARER'
printf '%s\n' 'STRICT_REDIRECT_POLICY=MANUAL_NO_FOLLOW_FAIL_CLOSED'
printf '%s\n' 'STRICT_SOCKET_POLICY=LOCAL_ERR_SOCKET_SESSION_NOT_READY_FALLBACK_ONLY'
printf '%s\n' 'TYPESCRIPT_BUILD_STATUS=PASS'
printf '%s\n' 'HOST_TEST_STATUS=PASS'
printf '%s\n' 'GIT_DIFF_CHECK=PASS'
printf '%s\n' 'REAL_HOMEY_READ_ONLY_CAPTURE=NOT_RUN'
printf '%s\n' 'HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED'
printf '%s\n' 'PASS: Patch040 host-only strict local read-only transport offline validation'
