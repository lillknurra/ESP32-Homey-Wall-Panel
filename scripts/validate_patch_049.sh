#!/usr/bin/env bash
set -euo pipefail
EXPECTED_BASE="40c8fe1f6935d917b7564c8888fcec8d2f523f63"
EXPECTED_TREE="cf3f1d1a3e2d487aac9e8bf45cdf20f9d326fde4"
EXPECTED_BRANCH="patch-049-volatile-homey-session-cache-immutable-oauth-store"
cd "$(git rev-parse --show-toplevel)"
fail(){ printf 'FAIL: %s\n' "$*" >&2; exit 1; }
req(){ grep -Fq -- "$1" "$2" || fail "missing $1 in $2"; }
test "$(git branch --show-current)" = "$EXPECTED_BRANCH" || fail branch
test "$(git rev-parse main)" = "$EXPECTED_BASE" || fail main
test "$(git rev-parse origin/main)" = "$EXPECTED_BASE" || fail origin
test "$(git rev-parse "$EXPECTED_BASE^{tree}")" = "$EXPECTED_TREE" || fail tree
E="$(mktemp)"; A="$(mktemp)"; trap 'rm -f "$E" "$A"' EXIT
printf '%s\n' docs/handoff/CURRENT_STATE.md docs/handoff/HANDOFF.md docs/handoff/MASTER_INDEX.md docs/history/PATCH_049_VOLATILE_HOMEY_SESSION_CACHE_IMMUTABLE_OAUTH_STORE.md docs/history/PATCH_HISTORY.md scripts/validate_patch_049.sh tools/homey-inventory/src/awning-athom-remote-candidates.ts tools/homey-inventory/test/awning-athom-remote-candidates.test.ts | sort > "$E"
{ git diff --name-only "$EXPECTED_BASE" HEAD; git diff --name-only; git ls-files --others --exclude-standard; } | sed '/^[[:space:]]*$/d' | sort -u > "$A"
diff -u "$E" "$A" || fail scope
SRC=tools/homey-inventory/src/awning-athom-remote-candidates.ts
TEST=tools/homey-inventory/test/awning-athom-remote-candidates.test.ts
req PATCH049_VOLATILE_HOMEY_SESSION_CACHE_CONTRACT "$SRC"
req createImmutableOauthVolatileHomeySessionStore "$SRC"
req 'Patch049 refuses Athom OAuth/account store mutation' "$SRC"
req 'volatile_process_memory_only' "$SRC"
req 'const store = createImmutableOauthVolatileHomeySessionStore(' "$SRC"
req 'Patch049 volatile store is memory-only and OAuth-immutable' "$TEST"
req 'PATCH049_STATUS=ACTIVE_IMPLEMENTATION_BRANCH__OFFLINE_VALIDATED_REMOTE_PUBLISHED' docs/handoff/CURRENT_STATE.md
req 'PATCH049_OFFLINE_VALIDATION=PASS__117_OF_117' docs/handoff/CURRENT_STATE.md
req 'PATCH049_IMPLEMENTATION_COMMIT=10ca8d5adb8032df3185dda077f6595e53006c82' docs/handoff/CURRENT_STATE.md
if grep -nE 'DISCOVERY_STRATEGIES\.(LOCAL|LOCAL_SECURE|MDNS)' "$SRC"; then fail local-strategy; fi
if grep -nE '\.(setCapabilityValue|runFlowCardAction|triggerFlow|triggerAdvancedFlow|genericApiCall)[[:space:]]*\(' "$SRC"; then fail mutation; fi
git diff --check
(cd tools/homey-inventory && npm run build && npm test)
printf '%s\n' PATCH049_OFFLINE_VALIDATION=PASS PATCH049_LIVE_ATHOM_ACCESS=NOT_RUN PATCH049_HOMEY_DEVICE_READ=NOT_RUN PATCH049_OAUTH_DISK_WRITE=NOT_RUN_AND_FORBIDDEN PATCH049_HOMEY_SESSION_PERSISTENCE=VOLATILE_PROCESS_MEMORY_ONLY PATCH049_HOMEY_MUTATION=NOT_RUN_AND_PROHIBITED
