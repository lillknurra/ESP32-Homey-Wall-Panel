#!/usr/bin/env bash
set -euo pipefail
ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"
BASE="5dcd50093a093b70758b6449911867f1d4428ad3"
EXPECTED_BRANCH="patch-009-offline-athom-session-parsing-homey-selection"
[[ "$(git branch --show-current)" == "$EXPECTED_BRANCH" ]] || { echo "FAIL: wrong branch"; exit 1; }
git merge-base --is-ancestor "$BASE" HEAD
bash -n scripts/validate_patch_009.sh
allowed='^(components/athom_cloud_native/include/athom_cloud_types.h|components/athom_cloud_native/include/athom_protocol.h|components/athom_cloud_native/src/athom_protocol.c|components/athom_cloud_native/src/athom_homey_client.c|components/athom_cloud_native/src/athom_http_esp.c|components/athom_cloud_native/test_host/test_executable_protocol.c|components/athom_cloud_native/test_host/test_athom_cloud_native.c|components/athom_cloud_native/test_host/test_live_transport.c|docs/architecture/ATHOM_CLOUD_NATIVE_ARCHITECTURE.md|docs/handoff/MASTER_INDEX.md|docs/handoff/CURRENT_STATE.md|docs/handoff/HANDOFF.md|docs/history/PATCH_HISTORY.md|docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md|scripts/validate_patch_009.sh)$'
changed="$(git status --short | sed -E 's/^.. //' | sed -E 's/.* -> //')"
while IFS= read -r f; do [[ -z "$f" || "$f" =~ $allowed ]] || { echo "FAIL: unexpected file $f"; exit 1; }; done <<< "$changed"
grep -q 'ATHOM_ERR_JSON_MALFORMED' components/athom_cloud_native/include/athom_cloud_types.h
grep -q 'ATHOM_ERR_JSON_DUPLICATE_KEY' components/athom_cloud_native/include/athom_cloud_types.h
grep -q 'ATHOM_ERR_HOMEY_SELECTION_REQUIRED' components/athom_cloud_native/include/athom_cloud_types.h
grep -q 'ATHOM_PROTOCOL_MAX_JSON_BYTES' components/athom_cloud_native/include/athom_protocol.h
grep -q 'duplicate' components/athom_cloud_native/src/athom_protocol.c
grep -q 'trailing' components/athom_cloud_native/src/athom_protocol.c
grep -q 'match_count' components/athom_cloud_native/src/athom_homey_client.c
grep -q 'session_homey_id' components/athom_cloud_native/src/athom_http_esp.c
grep -q 'allow_reauth' components/athom_cloud_native/src/athom_http_esp.c
grep -q 'return false;' components/athom_cloud_native/src/athom_homey_client.c
git diff --check
for forbidden in sdkconfig.defaults main/main.c components/secure_bootstrap; do
  if git diff --name-only "$BASE" -- "$forbidden" | grep -q .; then echo "FAIL: forbidden scope $forbidden"; exit 1; fi
done
if grep -RInE '(sk-[A-Za-z0-9]{20,}|Bearer [A-Za-z0-9._-]{20,}|Basic [A-Za-z0-9+/=]{20,})' \
  components/athom_cloud_native docs/history/PATCH_009_OFFLINE_ATHOM_SESSION_PARSING_AND_HOMEY_SELECTION.md; then
  echo "FAIL: secret-like value"; exit 1
fi
echo "PASS: Patch 009 static validator"
