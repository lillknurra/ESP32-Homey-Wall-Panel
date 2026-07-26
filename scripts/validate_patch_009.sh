#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
BASE="5dcd50093a093b70758b6449911867f1d4428ad3"
BRANCH="patch-009-offline-athom-session-parsing-homey-selection"
[[ "$(git branch --show-current)" == "$BRANCH" ]] || { echo "FAIL: wrong branch"; exit 1; }
git merge-base --is-ancestor "$BASE" HEAD
bash -n scripts/validate_patch_009.sh
allowed='^(components/athom_cloud_native/include/athom_cloud_types.h|components/athom_cloud_native/include/athom_protocol.h|components/athom_cloud_native/include/athom_http_transport.h|components/athom_cloud_native/src/athom_protocol.c|components/athom_cloud_native/src/athom_homey_client.c|components/athom_cloud_native/src/athom_http_esp.c|components/athom_cloud_native/test_host/test_executable_protocol.c|components/athom_cloud_native/test_host/test_athom_cloud_native.c|components/athom_cloud_native/test_host/test_live_transport.c|docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md|docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md|scripts/validate_patch_009.sh)$'
git diff --name-only "$BASE" | while IFS= read -r f; do [[ "$f" =~ $allowed ]] || { echo "FAIL: unexpected file $f"; exit 1; }; done
grep -q 'invalidate_session' components/athom_cloud_native/include/athom_http_transport.h
grep -q 'finish_document' components/athom_cloud_native/src/athom_protocol.c
grep -q 'ATHOM_ERR_JSON_DUPLICATE_KEY' components/athom_cloud_native/src/athom_protocol.c
grep -q 'athom_secure_zero(token_type' components/athom_cloud_native/src/athom_protocol.c
grep -q 'INT64_MAX - expires_in' components/athom_cloud_native/src/athom_protocol.c
grep -q 'parsed_connections' components/athom_cloud_native/src/athom_protocol.c
grep -q 'invalidate_session(ctx->transport)' components/athom_cloud_native/src/athom_homey_client.c
grep -q 'force_401_count=2' components/athom_cloud_native/test_host/test_live_transport.c
git diff --check
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT
INC='-Icomponents/athom_cloud_native/include'
cc -std=c11 -Wall -Wextra -Werror $INC components/athom_cloud_native/test_host/test_executable_protocol.c components/athom_cloud_native/src/athom_protocol.c components/athom_cloud_native/src/athom_runtime.c components/athom_cloud_native/src/athom_redaction.c -o "$TMP/protocol"
"$TMP/protocol"
cc -std=c11 -Wall -Wextra -Werror $INC components/athom_cloud_native/test_host/test_athom_cloud_native.c components/athom_cloud_native/src/athom_auth.c components/athom_cloud_native/src/athom_homey_client.c components/athom_cloud_native/src/athom_provisioning.c components/athom_cloud_native/src/athom_redaction.c -o "$TMP/native"
"$TMP/native"
cc -std=c11 -Wall -Wextra -Werror $INC components/athom_cloud_native/test_host/test_live_transport.c components/athom_cloud_native/src/athom_http_esp.c components/athom_cloud_native/src/athom_endpoints.c components/athom_cloud_native/src/athom_protocol.c components/athom_cloud_native/src/athom_redaction.c -o "$TMP/transport"
"$TMP/transport"
if grep -RInE '(sk-[A-Za-z0-9]{20,}|Bearer [A-Za-z0-9._-]{20,}|Basic [A-Za-z0-9+/=]{20,})' components/athom_cloud_native docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md; then echo 'FAIL: secret-like value'; exit 1; fi
echo 'PASS: Patch 009 executable validator'
