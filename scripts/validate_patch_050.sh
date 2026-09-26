#!/usr/bin/env bash
set -euo pipefail
export GIT_PAGER=cat
export PAGER=cat
export LESS=FRX
EXPECTED_BASE="bfa3b2421ad2f75d512e709d77c4357027d621de"
EXPECTED_BRANCH="patch-050-bounded-volatile-athom-oauth-refresh"
cd "$(git rev-parse --show-toplevel)"
fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
req(){ grep -Fq -- "$1" "$2" || fail "missing $1 in $2"; }
test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail branch
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail origin_main
test "$(git merge-base "$EXPECTED_BASE" HEAD)" = "$EXPECTED_BASE" || fail merge_base
test -z "$(git rev-list --merges "$EXPECTED_BASE"..HEAD)" || fail merge_commit
NODE_MAJOR="$(node -p 'Number(process.versions.node.split(".")[0])')"
test "$NODE_MAJOR" -ge 24 || fail "Node 24 or newer is required; observed major=$NODE_MAJOR"
E="$(mktemp)"; A="$(mktemp)"; trap 'rm -f "$E" "$A"' EXIT
printf '%s\n' \
  docs/handoff/CURRENT_STATE.md \
  docs/handoff/HANDOFF.md \
  docs/handoff/MASTER_INDEX.md \
  docs/history/PATCH_050_BOUNDED_VOLATILE_ATHOM_OAUTH_REFRESH.md \
  docs/history/PATCH_HISTORY.md \
  scripts/run_patch_043_remote_awning_candidates.sh \
  scripts/validate_patch_050.sh \
  tools/homey-inventory/src/awning-athom-remote-candidates.ts \
  tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$E"
{ git diff --name-only "$EXPECTED_BASE" HEAD; git diff --name-only; git ls-files --others --exclude-standard; } | sed '/^[[:space:]]*$/d' | sort -u > "$A"
diff -u "$E" "$A" || fail scope
test "$(wc -l < "$A" | tr -d ' ')" = "9" || fail file_count
SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
RUNNER=scripts/run_patch_043_remote_awning_candidates.sh
req PATCH050_BOUNDED_VOLATILE_OAUTH_REFRESH_CONTRACT "$SRC"
req 'trigger_status_code: 401' "$SRC"
req 'maximum_refresh_attempts: 1' "$SRC"
req 'athom_auto_refresh_tokens: false' "$SRC"
req createBoundedVolatileOauthRefreshStore "$SRC"
req listStoredOauthHomeysWithBoundedVolatileRefresh "$SRC"
req 'Patch050 refuses unarmed or repeated OAuth token rotation' "$SRC"
req 'Patch050 refuses more than one OAuth refresh attempt per runtime' "$SRC"
req 'authenticateWithRefreshToken()' "$SRC"
req resolvePatch050OauthClientConfig "$SRC"
req 'clientId: oauthClient.clientId' "$SRC"
req 'clientSecret: oauthClient.clientSecret' "$SRC"
req 'ATHOM_API_CLIENT_ID' "$SRC"
req 'ATHOM_API_CLIENT_SECRET' "$SRC"
req 'autoRefreshTokens: false' "$SRC"
req 'PATCH050_OAUTH_REFRESH=HTTP_401_ONLY__ONE_VOLATILE_ATTEMPT' "$RUNNER"
req 'PATCH050_OAUTH_CLIENT_CONFIG=EXTERNAL_PROCESS_ENVIRONMENT_REQUIRED' "$RUNNER"
req 'Patch050 OAuth client configuration is external, bounded and fail-closed' "$TEST"
req 'Patch050 store permits exactly one armed volatile OAuth rotation and never writes disk' "$TEST"
req 'Patch050 runtime refreshes exactly once on authenticated-user HTTP 401 and keeps disk OAuth immutable' "$TEST"
req 'Patch050 runtime does not refresh non-401 authenticated-user failures' "$TEST"
req 'Patch050 runtime does not refresh Homey-list HTTP 401' "$TEST"
req 'Patch050 runtime never performs a second refresh after a second HTTP 401' "$TEST"
req 'PATCH050_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATION_PENDING' docs/handoff/CURRENT_STATE.md
req 'PATCH_050_BOUNDED_VOLATILE_ATHOM_OAUTH_REFRESH.md' docs/handoff/MASTER_INDEX.md
if grep -Fq 'PATCH050_HOMEY_CLI_PUBLIC_OAUTH_CLIENT = Object.freeze' "$SRC"; then fail embedded_oauth_client_config; fi
if grep -nE '^[[:space:]]+client_(id|secret):[[:space:]]*"' "$SRC"; then fail embedded_oauth_client_value; fi
if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|testFlow|triggerAdvancedFlow|activateMood|createFlow|updateFlow|deleteFlow|createAdvancedFlow|updateAdvancedFlow|deleteAdvancedFlow|pair|unpair|genericApiCall)[[:space:]]*\(' "$SRC"; then fail mutation; fi
if grep -nE '\b(fetch|axios|https?\.request|http\.request)[[:space:]]*\(' "$SRC"; then fail direct_network; fi
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then fail local_strategy; fi
bash -n "$RUNNER"
bash -n scripts/validate_patch_050.sh
git diff --check
(
  cd tools/homey-inventory
  npm run build
  npm test
)
printf '%s\n' PATCH050_OFFLINE_VALIDATION=PASS PATCH050_LIVE_REFRESH=NOT_RUN PATCH050_HOMEY_DEVICE_READ=NOT_RUN PATCH050_OAUTH_DISK_WRITE=NOT_RUN_AND_FORBIDDEN PATCH050_OAUTH_TOKEN_PERSISTENCE=VOLATILE_PROCESS_MEMORY_ONLY PATCH050_AUTO_REFRESH_TOKENS=false PATCH050_BROWSER_LOGIN=NOT_RUN_AND_FORBIDDEN PATCH050_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED
